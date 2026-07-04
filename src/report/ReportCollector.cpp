#include "adasdf/report/ReportCollector.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "adasdf/report/ReportValidator.h"
#include "adasdf/report/StrictCsvWriter.h"
#include "adasdf/report/StrictJsonWriter.h"

namespace adasdf {
namespace {

std::string shellLikeArgv(int argc, char** argv) {
  std::ostringstream out;
  for (int i = 0; i < argc; ++i) {
    if (i > 0) {
      out << " ";
    }
    const std::string arg = argv[i] ? argv[i] : "";
    const bool needs_quotes =
        arg.find_first_of(" \t\"") != std::string::npos;
    if (!needs_quotes) {
      out << arg;
      continue;
    }
    out << "\"";
    for (const char ch : arg) {
      if (ch == '"' || ch == '\\') {
        out << "\\";
      }
      out << ch;
    }
    out << "\"";
  }
  return out.str();
}

}  // namespace

StrictRunTimer startStrictRunTimer() {
  StrictRunTimer timer;
  timer.wall_start = std::chrono::system_clock::now();
  timer.steady_start = std::chrono::steady_clock::now();
  return timer;
}

std::map<std::string, std::string> commandLineParameters(
    int argc,
    char** argv) {
  std::map<std::string, std::string> result;
  result["argc"] = std::to_string(argc);
  result["argv"] = shellLikeArgv(argc, argv);
  return result;
}

bool writeStrictRunReport(
    const std::filesystem::path& strict_json_path,
    const std::string& tool_name,
    const std::string& case_id,
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path,
    const std::map<std::string, std::string>& parameters,
    const std::map<std::string, double>& metrics,
    bool success,
    const std::string& status,
    const std::string& failure_reason,
    const StrictRunTimer& timer,
    std::string* error_message) {
  if (strict_json_path.empty()) {
    return true;
  }
  const auto steady_end = std::chrono::steady_clock::now();
  const auto wall_end = std::chrono::system_clock::now();
  RunManifest manifest =
      makeRunManifest(tool_name, case_id, input_path, output_path);
  manifest.parameters = parameters;
  manifest.metrics = metrics;
  manifest.status = status;
  manifest.success = success;
  manifest.failure_reason = failure_reason;
  manifest.start_time_utc = utcTimestamp(timer.wall_start);
  manifest.end_time_utc = utcTimestamp(wall_end);
  manifest.elapsed_ms =
      std::chrono::duration<double, std::milli>(
          steady_end - timer.steady_start)
          .count();
  return StrictJsonWriter::writeFile(strict_json_path, manifest, error_message);
}

ReportCollectorResult ReportCollector::collectList(
    const std::filesystem::path& inputs) {
  ReportCollectorResult result;
  std::ifstream list(inputs);
  if (!list) {
    result.error_message = "failed to read report list: " + inputs.string();
    return result;
  }
  const std::filesystem::path base = inputs.parent_path();
  std::string line;
  int line_number = 0;
  while (std::getline(list, line)) {
    ++line_number;
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::filesystem::path report_path(line);
    if (report_path.is_relative()) {
      report_path = base / report_path;
    }
    ReportValidationResult validation =
        ReportValidator::validateFile(report_path);
    if (!validation.readable) {
      result.error_message =
          "line " + std::to_string(line_number) + ": " +
          validation.errors.front();
      return result;
    }
    if (!validation.valid) {
      std::ostringstream message;
      message << "line " << line_number << ": invalid report";
      for (const std::string& error : validation.errors) {
        message << "; " << error;
      }
      result.error_message = message.str();
      return result;
    }
    result.records.push_back(std::move(validation.record));
  }
  result.success = true;
  return result;
}

bool ReportCollector::collectRunSummary(
    const std::filesystem::path& inputs,
    const std::filesystem::path& output_csv,
    std::string* error_message) {
  ReportCollectorResult result = collectList(inputs);
  if (!result.success) {
    if (error_message) {
      *error_message = result.error_message;
    }
    return false;
  }
  return StrictCsvWriter::writeSummaryCSV(
      output_csv,
      result.records,
      error_message);
}

}  // namespace adasdf


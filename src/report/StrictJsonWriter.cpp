#include "adasdf/report/StrictJsonWriter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace adasdf {
namespace {

void writeStringField(
    std::ostream& out,
    const std::string& name,
    const std::string& value,
    bool comma = true) {
  out << "  \"" << name << "\": \"" << StrictJsonWriter::escape(value) << "\"";
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeNumberField(
    std::ostream& out,
    const std::string& name,
    double value,
    bool comma = true) {
  out << "  \"" << name << "\": "
      << std::setprecision(std::numeric_limits<double>::max_digits10)
      << value;
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeBoolField(
    std::ostream& out,
    const std::string& name,
    bool value,
    bool comma = true) {
  out << "  \"" << name << "\": " << (value ? "true" : "false");
  if (comma) {
    out << ",";
  }
  out << "\n";
}

}  // namespace

std::string StrictJsonWriter::escape(const std::string& value) {
  std::ostringstream out;
  for (const unsigned char ch : value) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (ch < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(ch) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  return out.str();
}

std::string StrictJsonWriter::toJson(const RunManifest& manifest) {
  std::ostringstream out;
  out << "{\n";
  writeStringField(out, "schema_version", manifest.schema_version);
  writeStringField(out, "adasdf_version", manifest.adasdf_version);
  writeStringField(out, "git_commit", manifest.git_commit);
  writeStringField(out, "tool_name", manifest.tool_name);
  writeStringField(out, "case_id", manifest.case_id);
  writeStringField(out, "input_path", pathToReportString(manifest.input_path));
  writeStringField(out, "input_sha256", manifest.input_sha256);
  writeStringField(out, "output_path", pathToReportString(manifest.output_path));
  writeStringField(out, "output_sha256", manifest.output_sha256);
  out << "  \"parameters\": {";
  if (!manifest.parameters.empty()) {
    out << "\n";
    std::size_t index = 0;
    for (const auto& item : manifest.parameters) {
      out << "    \"" << escape(item.first) << "\": \""
          << escape(item.second) << "\"";
      if (++index < manifest.parameters.size()) {
        out << ",";
      }
      out << "\n";
    }
    out << "  ";
  }
  out << "},\n";
  writeStringField(out, "status", manifest.status);
  writeBoolField(out, "success", manifest.success);
  writeStringField(out, "failure_reason", manifest.failure_reason);
  writeStringField(out, "start_time_utc", manifest.start_time_utc);
  writeStringField(out, "end_time_utc", manifest.end_time_utc);
  writeNumberField(out, "elapsed_ms", manifest.elapsed_ms);
  writeStringField(out, "platform", manifest.platform);
  out << "  \"cpu_threads\": " << manifest.cpu_threads << ",\n";
  writeBoolField(out, "cuda_enabled", manifest.cuda_enabled);
  writeBoolField(out, "cuda_available", manifest.cuda_available,
                 !manifest.metrics.empty());

  std::size_t metric_index = 0;
  for (const auto& metric : manifest.metrics) {
    writeNumberField(
        out,
        metric.first,
        metric.second,
        ++metric_index < manifest.metrics.size());
  }
  out << "}\n";
  return out.str();
}

bool StrictJsonWriter::writeFile(
    const std::filesystem::path& path,
    const RunManifest& manifest,
    std::string* error_message) {
  try {
    if (!path.parent_path().empty()) {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
      if (error_message) {
        *error_message = "failed to open strict JSON output: " + path.string();
      }
      return false;
    }
    file << toJson(manifest);
    return true;
  } catch (const std::exception& exc) {
    if (error_message) {
      *error_message = exc.what();
    }
    return false;
  }
}

}  // namespace adasdf

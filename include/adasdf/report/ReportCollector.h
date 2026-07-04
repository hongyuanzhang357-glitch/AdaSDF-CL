#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "adasdf/report/RunManifest.h"

namespace adasdf {

struct StrictRunTimer {
  std::chrono::system_clock::time_point wall_start;
  std::chrono::steady_clock::time_point steady_start;
};

StrictRunTimer startStrictRunTimer();

std::map<std::string, std::string> commandLineParameters(
    int argc,
    char** argv);

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
    std::string* error_message = nullptr);

struct ReportCollectorResult {
  bool success = false;
  std::string error_message;
  std::vector<StrictReportRecord> records;
};

class ReportCollector {
 public:
  static ReportCollectorResult collectList(const std::filesystem::path& inputs);
  static bool collectRunSummary(
      const std::filesystem::path& inputs,
      const std::filesystem::path& output_csv,
      std::string* error_message = nullptr);
};

}  // namespace adasdf


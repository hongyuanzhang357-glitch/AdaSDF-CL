#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <string>

#include "adasdf/report/StrictReportSchema.h"

namespace adasdf {

struct RunManifest {
  std::string schema_version;
  std::string adasdf_version;
  std::string git_commit;
  std::string tool_name;
  std::string case_id;
  std::filesystem::path input_path;
  std::string input_sha256;
  std::filesystem::path output_path;
  std::string output_sha256;
  std::map<std::string, std::string> parameters;
  std::string status;
  bool success = false;
  std::string failure_reason;
  std::string start_time_utc;
  std::string end_time_utc;
  double elapsed_ms = 0.0;
  std::string platform;
  unsigned int cpu_threads = 0;
  bool cuda_enabled = false;
  bool cuda_available = false;
  std::map<std::string, double> metrics;
};

std::string utcTimestamp(std::chrono::system_clock::time_point time);
std::string currentUtcTimestamp();
std::string currentGitCommit();
std::string currentPlatformName();
unsigned int currentCpuThreads();
bool buildCudaEnabled();
bool buildCudaAvailable();
std::string fileSha256(const std::filesystem::path& path);
std::string pathToReportString(const std::filesystem::path& path);

RunManifest makeRunManifest(
    const std::string& tool_name,
    const std::string& case_id,
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path);

StrictReportRecord toStrictReportRecord(const RunManifest& manifest);

}  // namespace adasdf


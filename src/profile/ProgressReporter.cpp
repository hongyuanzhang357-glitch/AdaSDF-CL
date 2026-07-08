#include "adasdf/profile/ProgressReporter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "adasdf/contract/JsonContractWriter.h"
#include "adasdf/contract/SchemaIds.h"
#include "adasdf/version.h"

namespace adasdf {

ProgressReporter::ProgressReporter(
    std::string tool_name,
    bool stderr_enabled,
    std::filesystem::path jsonl_path)
    : tool_name_(std::move(tool_name)),
      stderr_enabled_(stderr_enabled),
      jsonl_path_(std::move(jsonl_path)) {}

bool ProgressReporter::enabled() const {
  return stderr_enabled_ || !jsonl_path_.empty();
}

void ProgressReporter::emit(
    const std::string& stage,
    const std::string& message,
    std::size_t current,
    std::size_t total) const {
  if (!enabled()) {
    return;
  }
  const double percent =
      total > 0 ? 100.0 * static_cast<double>(current) /
                      static_cast<double>(total)
                : 0.0;
  const std::string line =
      "{\"schema_id\":" + JsonContractWriter::quote(SchemaIds::Progress) +
      ",\"schema_version\":1,\"adasdf_version\":" +
      JsonContractWriter::quote(versionString()) + ",\"tool_name\":" +
      JsonContractWriter::quote(tool_name_) + ",\"generated_at_utc\":" +
      JsonContractWriter::quote(JsonContractWriter::generatedAtUtc()) +
      ",\"stage\":" + JsonContractWriter::quote(stage) +
      ",\"message\":" + JsonContractWriter::quote(message) +
      ",\"current\":" + JsonContractWriter::integer(current) +
      ",\"total\":" + JsonContractWriter::integer(total) +
      ",\"percent\":" + JsonContractWriter::number(percent) +
      ",\"elapsed_ms\":0}\n";
  if (stderr_enabled_) {
    std::cerr << line;
  }
  if (!jsonl_path_.empty()) {
    if (!jsonl_path_.parent_path().empty()) {
      std::filesystem::create_directories(jsonl_path_.parent_path());
    }
    std::ofstream file(jsonl_path_, std::ios::app);
    if (file) {
      file << line;
    }
  }
}

}  // namespace adasdf

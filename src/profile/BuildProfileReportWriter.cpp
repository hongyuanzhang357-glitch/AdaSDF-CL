#include "adasdf/profile/BuildProfileReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "adasdf/contract/ErrorCode.h"

namespace adasdf {

std::string BuildProfileReportWriter::toMarkdown(
    const BuildProfiler& profiler) {
  std::ostringstream out;
  out << "# AdaSDF-CL Build Profile\n\n";
  out << "- Status: " << profiler.status << "\n";
  out << "- Status code: " << toString(profiler.status_code) << "\n";
  out << "- Tool: " << profiler.tool_name << "\n";
  out << "- Input: " << profiler.input_path << "\n";
  out << "- Output: " << profiler.output_path << "\n";
  out << "- Total time ms: " << profiler.timings.total_time_ms << "\n";
  out << "- Blocks: " << profiler.counters.num_blocks << "\n";
  out << "- Compressed blocks: " << profiler.counters.compressed_block_count
      << "\n";
  out << "- Dense fallback blocks: "
      << profiler.counters.dense_fallback_block_count << "\n";
  return out.str();
}

bool BuildProfileReportWriter::writeMarkdown(
    const std::string& path,
    const BuildProfiler& profiler) {
  if (path.empty()) {
    return true;
  }
  const std::filesystem::path output(path);
  if (!output.parent_path().empty()) {
    std::filesystem::create_directories(output.parent_path());
  }
  std::ofstream file(output);
  if (!file) {
    return false;
  }
  file << toMarkdown(profiler);
  return static_cast<bool>(file);
}

}  // namespace adasdf

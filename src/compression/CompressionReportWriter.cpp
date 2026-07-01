#include "adasdf/compression/CompressionReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {
namespace {

std::string boolText(bool value) {
  return value ? "true" : "false";
}

void writeText(const std::string& path, const std::string& text) {
  if (path.empty()) {
    return;
  }
  const std::filesystem::path output(path);
  if (output.has_parent_path()) {
    std::filesystem::create_directories(output.parent_path());
  }
  std::ofstream file(output, std::ios::trunc);
  file << text;
}

}  // namespace

std::string CompressionReportWriter::toMarkdown(
    const BlockLowRankCompressionReport& report) {
  std::ostringstream out;
  out << "# AdaSDF-CL Low-Rank Block Compression Report\n\n";
  out << "## Summary\n\n";
  out << "- Success: " << boolText(report.success) << "\n";
  out << "- Input blocks: " << report.input_block_count << "\n";
  out << "- Matrix-SVD blocks: " << report.compressed_block_count << "\n";
  out << "- Dense fallback blocks: " << report.dense_fallback_block_count << "\n";
  out << "- Near-surface blocks: " << report.near_surface_block_count << "\n";
  out << "- Original memory bytes: " << report.original_memory_bytes << "\n";
  out << "- Compressed memory bytes: " << report.compressed_memory_bytes << "\n";
  out << "- Compression ratio: " << report.compression_ratio << "\n";
  out << "- Compression time ms: " << report.compression_time_ms << "\n\n";
  out << "## Error Metrics\n\n";
  out << "- Max abs error: " << report.global_max_abs_error << "\n";
  out << "- Mean abs error: " << report.global_mean_abs_error << "\n";
  out << "- RMS error: " << report.global_rms_error << "\n";
  out << "- P95 abs error: " << report.global_p95_abs_error << "\n";
  out << "- Sign mismatches: " << report.sign_mismatch_count << "\n";
  out << "- Near-surface sign mismatches: "
      << report.near_surface_sign_mismatch_count << "\n\n";
  out << "## Ranks Used\n\n";
  for (int rank : report.ranks_used) {
    out << "- " << rank << "\n";
  }
  if (report.ranks_used.empty()) {
    out << "- none\n";
  }
  out << "\n## Limitations\n\n";
  out << "- Matrix-SVD block compression is implemented.\n";
  out << "- Dense fallback is used when compression cannot satisfy the target.\n";
  out << "- Tucker/HOSVD compression is planned, not implemented.\n";
  out << "- Surrogate recommendation remains planned for v1.8.\n";
  out << "- GPU-native compressed query remains planned.\n\n";
  if (!report.warnings.empty()) {
    out << "## Warnings\n\n";
    for (const std::string& warning : report.warnings) {
      out << "- " << warning << "\n";
    }
  }
  return out.str();
}

std::string CompressionReportWriter::toJson(
    const BlockLowRankCompressionReport& report) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(report.success) << ",\n";
  out << "  \"input_block_count\": " << report.input_block_count << ",\n";
  out << "  \"compressed_block_count\": " << report.compressed_block_count << ",\n";
  out << "  \"dense_fallback_block_count\": "
      << report.dense_fallback_block_count << ",\n";
  out << "  \"original_memory_bytes\": " << report.original_memory_bytes << ",\n";
  out << "  \"compressed_memory_bytes\": " << report.compressed_memory_bytes << ",\n";
  out << "  \"compression_ratio\": " << report.compression_ratio << ",\n";
  out << "  \"global_max_abs_error\": " << report.global_max_abs_error << ",\n";
  out << "  \"global_rms_error\": " << report.global_rms_error << ",\n";
  out << "  \"global_p95_abs_error\": " << report.global_p95_abs_error << ",\n";
  out << "  \"sign_mismatch_count\": " << report.sign_mismatch_count << "\n";
  out << "}\n";
  return out.str();
}

std::string CompressionReportWriter::qualityToMarkdown(
    const CompressionQualityReport& report) {
  std::ostringstream out;
  out << "# AdaSDF-CL Compression Quality Report\n\n";
  out << "- Success: " << boolText(report.success) << "\n";
  out << "- Samples: " << report.sample_count << "\n";
  out << "- Max abs error: " << report.max_abs_error << "\n";
  out << "- Mean abs error: " << report.mean_abs_error << "\n";
  out << "- RMS error: " << report.rms_error << "\n";
  out << "- P95 abs error: " << report.p95_abs_error << "\n";
  out << "- Sign mismatches: " << report.sign_mismatch_count << "\n";
  out << "- Near-surface sign mismatches: "
      << report.near_surface_sign_mismatch_count << "\n";
  out << "- Compression ratio: " << report.compression_ratio << "\n";
  return out.str();
}

std::string CompressionReportWriter::qualityToJson(
    const CompressionQualityReport& report) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << boolText(report.success) << ",\n";
  out << "  \"sample_count\": " << report.sample_count << ",\n";
  out << "  \"max_abs_error\": " << report.max_abs_error << ",\n";
  out << "  \"mean_abs_error\": " << report.mean_abs_error << ",\n";
  out << "  \"rms_error\": " << report.rms_error << ",\n";
  out << "  \"p95_abs_error\": " << report.p95_abs_error << ",\n";
  out << "  \"sign_mismatch_count\": " << report.sign_mismatch_count << "\n";
  out << "}\n";
  return out.str();
}

void CompressionReportWriter::writeMarkdown(
    const std::string& path,
    const BlockLowRankCompressionReport& report) {
  writeText(path, toMarkdown(report));
}

void CompressionReportWriter::writeJson(
    const std::string& path,
    const BlockLowRankCompressionReport& report) {
  writeText(path, toJson(report));
}

void CompressionReportWriter::writeQualityMarkdown(
    const std::string& path,
    const CompressionQualityReport& report) {
  writeText(path, qualityToMarkdown(report));
}

void CompressionReportWriter::writeQualityJson(
    const std::string& path,
    const CompressionQualityReport& report) {
  writeText(path, qualityToJson(report));
}

}  // namespace adasdf

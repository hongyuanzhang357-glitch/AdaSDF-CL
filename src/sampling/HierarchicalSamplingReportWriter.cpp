#include "adasdf/sampling/HierarchicalSamplingReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {
namespace {

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string escaped(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

}  // namespace

std::string HierarchicalSamplingReportWriter::toMarkdown(
    const SpeedQualityBenchmarkResult& result) {
  std::ostringstream out;
  out << "# Hierarchical Adaptive Sampling Benchmark\n\n";
  out << "- Success: " << (result.success ? "yes" : "no") << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "- Exact build time ms: " << result.exact_build_time_ms << "\n";
  out << "- Hierarchical build time ms: "
      << result.hierarchical_build_time_ms << "\n";
  out << "- Speedup: " << result.speedup << "\n";
  out << "- Max abs error: " << result.max_abs_error << "\n";
  out << "- Mean abs error: " << result.mean_abs_error << "\n";
  out << "- RMS error: " << result.rms_error << "\n";
  out << "- P95 error: " << result.p95_error << "\n";
  out << "- Sign mismatches: " << result.sign_mismatch_count << "\n";
  out << "- Near-surface sign mismatches: "
      << result.near_surface_sign_mismatch_count << "\n";
  out << "- Exact sample count: " << result.exact_sample_count << "\n";
  out << "- Hierarchical exact sample count: "
      << result.hierarchical_exact_sample_count << "\n";
  out << "- Predicted sample count: " << result.predicted_sample_count << "\n";
  out << "- Fallback block count: " << result.fallback_block_count << "\n";
  out << "- Quality gate passed: "
      << (result.quality_gate_passed ? "yes" : "no") << "\n\n";
  out << "Near-surface blocks remain exact by default. Predicted transition "
         "and far-field blocks are accepted only after quality checks, with "
         "exact fallback when the guard fails.\n";
  return out.str();
}

std::string HierarchicalSamplingReportWriter::toJson(
    const SpeedQualityBenchmarkResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
  out << "  \"error_message\": \"" << escaped(result.error_message) << "\",\n";
  out << "  \"exact_build_time_ms\": " << result.exact_build_time_ms << ",\n";
  out << "  \"hierarchical_build_time_ms\": "
      << result.hierarchical_build_time_ms << ",\n";
  out << "  \"speedup\": " << result.speedup << ",\n";
  out << "  \"max_abs_error\": " << result.max_abs_error << ",\n";
  out << "  \"mean_abs_error\": " << result.mean_abs_error << ",\n";
  out << "  \"rms_error\": " << result.rms_error << ",\n";
  out << "  \"p95_error\": " << result.p95_error << ",\n";
  out << "  \"sign_mismatch_count\": " << result.sign_mismatch_count << ",\n";
  out << "  \"near_surface_sign_mismatch_count\": "
      << result.near_surface_sign_mismatch_count << ",\n";
  out << "  \"exact_sample_count\": " << result.exact_sample_count << ",\n";
  out << "  \"hierarchical_exact_sample_count\": "
      << result.hierarchical_exact_sample_count << ",\n";
  out << "  \"predicted_sample_count\": " << result.predicted_sample_count
      << ",\n";
  out << "  \"fallback_block_count\": " << result.fallback_block_count
      << ",\n";
  out << "  \"quality_gate_passed\": "
      << (result.quality_gate_passed ? "true" : "false") << "\n";
  out << "}\n";
  return out.str();
}

std::string HierarchicalSamplingReportWriter::csvHeader() {
  return "exact_build_time_ms,hierarchical_build_time_ms,speedup,"
         "max_abs_error,mean_abs_error,rms_error,p95_error,"
         "sign_mismatch_count,near_surface_sign_mismatch_count,"
         "exact_sample_count,hierarchical_exact_sample_count,"
         "predicted_sample_count,fallback_block_count,quality_gate_passed,"
         "status,error_message";
}

std::string HierarchicalSamplingReportWriter::csvRow(
    const SpeedQualityBenchmarkResult& result) {
  std::ostringstream out;
  out << result.exact_build_time_ms << ","
      << result.hierarchical_build_time_ms << "," << result.speedup << ","
      << result.max_abs_error << "," << result.mean_abs_error << ","
      << result.rms_error << "," << result.p95_error << ","
      << result.sign_mismatch_count << ","
      << result.near_surface_sign_mismatch_count << ","
      << result.exact_sample_count << ","
      << result.hierarchical_exact_sample_count << ","
      << result.predicted_sample_count << "," << result.fallback_block_count
      << "," << (result.quality_gate_passed ? "true" : "false") << ","
      << (result.success ? "ok" : "failed") << ",\"" << escaped(result.error_message)
      << "\"";
  return out.str();
}

void HierarchicalSamplingReportWriter::writeMarkdown(
    const std::string& path_string,
    const SpeedQualityBenchmarkResult& result) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toMarkdown(result);
}

void HierarchicalSamplingReportWriter::writeJson(
    const std::string& path_string,
    const SpeedQualityBenchmarkResult& result) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toJson(result);
}

void HierarchicalSamplingReportWriter::writeCsv(
    const std::string& path_string,
    const SpeedQualityBenchmarkResult& result) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << csvHeader() << "\n" << csvRow(result) << "\n";
}

}  // namespace adasdf

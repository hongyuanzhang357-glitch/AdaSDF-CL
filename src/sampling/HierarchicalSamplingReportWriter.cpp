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
  if (!result.case_id.empty()) {
    out << "- Case id: " << result.case_id << "\n";
  }
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
  out << "## Diagnostics\n\n";
  out << "- Effective speedup claim allowed: "
      << (result.effective_speedup_claim_allowed ? "yes" : "no") << "\n";
  out << "- Far-field quality check: "
      << toString(result.far_field_quality_check) << "\n";
  out << "- Far-field safety factor: "
      << result.far_field_safety_factor << "\n";
  out << "- Total blocks: " << result.diagnostics.total_block_count << "\n";
  out << "- Near-surface blocks: "
      << result.diagnostics.near_surface_block_count << "\n";
  out << "- Transition blocks: "
      << result.diagnostics.transition_block_count << "\n";
  out << "- Far-field blocks: "
      << result.diagnostics.far_field_block_count << "\n";
  out << "- Coarse samples: "
      << result.diagnostics.coarse_sample_count << "\n";
  out << "- Reused coarse samples: "
      << result.diagnostics.reused_coarse_sample_count << "\n";
  out << "- Exact BVH samples: "
      << result.diagnostics.exact_bvh_sample_count << "\n";
  out << "- Quality check samples: "
      << result.diagnostics.quality_check_sample_count << "\n";
  out << "- Exact sample reduction ratio: "
      << result.diagnostics.exact_sample_reduction_ratio << "\n";
  out << "- Prediction acceptance rate: "
      << result.diagnostics.prediction_acceptance_rate << "\n";
  out << "- Fallback rate: " << result.diagnostics.fallback_rate << "\n";
  out << "- Classification time ms: "
      << result.diagnostics.classification_time_ms << "\n";
  out << "- Coarse sampling time ms: "
      << result.diagnostics.coarse_sampling_time_ms << "\n";
  out << "- Prediction time ms: "
      << result.diagnostics.prediction_time_ms << "\n";
  out << "- Quality check time ms: "
      << result.diagnostics.quality_check_time_ms << "\n";
  out << "- Fallback exact time ms: "
      << result.diagnostics.fallback_exact_time_ms << "\n";
  out << "- Exact sampling time ms: "
      << result.diagnostics.exact_sampling_time_ms << "\n\n";
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
  out << "  \"case_id\": \"" << escaped(result.case_id) << "\",\n";
  out << "  \"max_level\": " << result.max_level << ",\n";
  out << "  \"block_resolution\": " << result.block_resolution << ",\n";
  out << "  \"coarse_resolution\": " << result.coarse_resolution << ",\n";
  out << "  \"quality_check_samples\": " << result.quality_check_samples << ",\n";
  out << "  \"transition_quality_check_samples\": "
      << result.transition_quality_check_samples << ",\n";
  out << "  \"far_field_quality_check\": \""
      << toString(result.far_field_quality_check) << "\",\n";
  out << "  \"far_field_safety_factor\": "
      << result.far_field_safety_factor << ",\n";
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
      << (result.quality_gate_passed ? "true" : "false") << ",\n";
  out << "  \"effective_speedup_claim_allowed\": "
      << (result.effective_speedup_claim_allowed ? "true" : "false")
      << ",\n";
  out << "  \"diagnostics\": {\n";
  out << "    \"total_block_count\": "
      << result.diagnostics.total_block_count << ",\n";
  out << "    \"near_surface_block_count\": "
      << result.diagnostics.near_surface_block_count << ",\n";
  out << "    \"transition_block_count\": "
      << result.diagnostics.transition_block_count << ",\n";
  out << "    \"far_field_block_count\": "
      << result.diagnostics.far_field_block_count << ",\n";
  out << "    \"exact_block_count\": "
      << result.diagnostics.exact_block_count << ",\n";
  out << "    \"predicted_block_count\": "
      << result.diagnostics.predicted_block_count << ",\n";
  out << "    \"accepted_prediction_block_count\": "
      << result.diagnostics.accepted_prediction_block_count << ",\n";
  out << "    \"fallback_exact_block_count\": "
      << result.diagnostics.fallback_exact_block_count << ",\n";
  out << "    \"coarse_sample_count\": "
      << result.diagnostics.coarse_sample_count << ",\n";
  out << "    \"fine_sample_count\": "
      << result.diagnostics.fine_sample_count << ",\n";
  out << "    \"exact_bvh_sample_count\": "
      << result.diagnostics.exact_bvh_sample_count << ",\n";
  out << "    \"predicted_sample_count\": "
      << result.diagnostics.predicted_sample_count << ",\n";
  out << "    \"quality_check_sample_count\": "
      << result.diagnostics.quality_check_sample_count << ",\n";
  out << "    \"reused_coarse_sample_count\": "
      << result.diagnostics.reused_coarse_sample_count << ",\n";
  out << "    \"skipped_far_field_quality_check_count\": "
      << result.diagnostics.skipped_far_field_quality_check_count << ",\n";
  out << "    \"distance_query_count\": "
      << result.diagnostics.distance_query_count << ",\n";
  out << "    \"sign_query_count\": "
      << result.diagnostics.sign_query_count << ",\n";
  out << "    \"classification_time_ms\": "
      << result.diagnostics.classification_time_ms << ",\n";
  out << "    \"coarse_sampling_time_ms\": "
      << result.diagnostics.coarse_sampling_time_ms << ",\n";
  out << "    \"prediction_time_ms\": "
      << result.diagnostics.prediction_time_ms << ",\n";
  out << "    \"quality_check_time_ms\": "
      << result.diagnostics.quality_check_time_ms << ",\n";
  out << "    \"fallback_exact_time_ms\": "
      << result.diagnostics.fallback_exact_time_ms << ",\n";
  out << "    \"exact_sampling_time_ms\": "
      << result.diagnostics.exact_sampling_time_ms << ",\n";
  out << "    \"total_hierarchical_time_ms\": "
      << result.diagnostics.total_hierarchical_time_ms << ",\n";
  out << "    \"exact_reference_time_ms\": "
      << result.diagnostics.exact_reference_time_ms << ",\n";
  out << "    \"speedup_vs_exact\": "
      << result.diagnostics.speedup_vs_exact << ",\n";
  out << "    \"exact_sample_reduction_ratio\": "
      << result.diagnostics.exact_sample_reduction_ratio << ",\n";
  out << "    \"prediction_acceptance_rate\": "
      << result.diagnostics.prediction_acceptance_rate << ",\n";
  out << "    \"fallback_rate\": "
      << result.diagnostics.fallback_rate << "\n";
  out << "  }\n";
  out << "}\n";
  return out.str();
}

std::string HierarchicalSamplingReportWriter::csvHeader() {
  return "exact_build_time_ms,hierarchical_build_time_ms,speedup,"
         "max_abs_error,mean_abs_error,rms_error,p95_error,"
         "sign_mismatch_count,near_surface_sign_mismatch_count,"
         "exact_sample_count,hierarchical_exact_sample_count,"
         "predicted_sample_count,fallback_block_count,quality_gate_passed,"
         "status,error_message,"
         "case_id,max_level,block_resolution,coarse_resolution,"
         "quality_check_samples,transition_quality_check_samples,"
         "far_field_quality_check,far_field_safety_factor,"
         "total_block_count,near_surface_block_count,transition_block_count,"
         "far_field_block_count,exact_block_count,predicted_block_count,"
         "accepted_prediction_block_count,fallback_exact_block_count,"
         "exact_bvh_sample_count,diagnostic_predicted_sample_count,"
         "quality_check_sample_count,coarse_sample_count,"
         "reused_coarse_sample_count,skipped_far_field_quality_check_count,"
         "distance_query_count,sign_query_count,classification_time_ms,"
         "coarse_sampling_time_ms,diagnostic_prediction_time_ms,"
         "diagnostic_quality_check_time_ms,fallback_exact_time_ms,"
         "diagnostic_exact_sampling_time_ms,total_hierarchical_time_ms,"
         "exact_reference_time_ms,speedup_vs_exact,"
         "exact_sample_reduction_ratio,prediction_acceptance_rate,"
         "fallback_rate,effective_speedup_claim_allowed";
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
      << "\",\"" << escaped(result.case_id) << "\"," << result.max_level
      << "," << result.block_resolution << "," << result.coarse_resolution
      << "," << result.quality_check_samples << ","
      << result.transition_quality_check_samples << ","
      << toString(result.far_field_quality_check) << ","
      << result.far_field_safety_factor << ","
      << result.diagnostics.total_block_count << ","
      << result.diagnostics.near_surface_block_count << ","
      << result.diagnostics.transition_block_count << ","
      << result.diagnostics.far_field_block_count << ","
      << result.diagnostics.exact_block_count << ","
      << result.diagnostics.predicted_block_count << ","
      << result.diagnostics.accepted_prediction_block_count << ","
      << result.diagnostics.fallback_exact_block_count << ","
      << result.diagnostics.exact_bvh_sample_count << ","
      << result.diagnostics.predicted_sample_count << ","
      << result.diagnostics.quality_check_sample_count << ","
      << result.diagnostics.coarse_sample_count << ","
      << result.diagnostics.reused_coarse_sample_count << ","
      << result.diagnostics.skipped_far_field_quality_check_count << ","
      << result.diagnostics.distance_query_count << ","
      << result.diagnostics.sign_query_count << ","
      << result.diagnostics.classification_time_ms << ","
      << result.diagnostics.coarse_sampling_time_ms << ","
      << result.diagnostics.prediction_time_ms << ","
      << result.diagnostics.quality_check_time_ms << ","
      << result.diagnostics.fallback_exact_time_ms << ","
      << result.diagnostics.exact_sampling_time_ms << ","
      << result.diagnostics.total_hierarchical_time_ms << ","
      << result.diagnostics.exact_reference_time_ms << ","
      << result.diagnostics.speedup_vs_exact << ","
      << result.diagnostics.exact_sample_reduction_ratio << ","
      << result.diagnostics.prediction_acceptance_rate << ","
      << result.diagnostics.fallback_rate << ","
      << (result.effective_speedup_claim_allowed ? "true" : "false");
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

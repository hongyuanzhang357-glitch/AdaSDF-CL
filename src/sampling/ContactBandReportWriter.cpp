#include "adasdf/sampling/ContactBandReportWriter.h"

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

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

}  // namespace

std::string ContactBandReportWriter::csvHeader() {
  return "case_id,exact_reference_time_ms,contact_band_time_ms,speedup,"
         "total_block_count,contact_band_block_count,far_field_block_count,"
         "total_node_count,exact_node_count,predicted_node_count,"
         "far_field_node_count,coarse_sample_count,exact_node_ratio,"
         "predicted_node_ratio,exact_sample_reduction_ratio,"
         "distance_query_count,sign_query_count,sign_query_reduction_ratio,"
         "contact_band_max_abs_error,contact_band_rms_error,"
         "contact_band_p95_error,contact_band_sign_mismatch_count,"
         "near_surface_sign_mismatch_count,mean_normal_angle_error_deg,"
         "p95_normal_angle_error_deg,max_normal_angle_error_deg,"
         "normal_flip_count,near_surface_normal_flip_count,"
         "contact_band_quality_passed,effective_speedup_claim_allowed";
}

std::string ContactBandReportWriter::csvRow(
    const ContactBandBenchmarkResult& result) {
  std::ostringstream out;
  const ContactBandDiagnostics& d = result.diagnostics;
  const ContactBandQualityMetrics& q = result.quality;
  out << result.case_id << "," << result.exact_reference_time_ms << ","
      << result.contact_band_time_ms << "," << result.speedup << ","
      << d.total_block_count << "," << d.contact_band_block_count << ","
      << d.far_field_block_count << "," << d.total_node_count << ","
      << d.exact_node_count << "," << d.predicted_node_count << ","
      << d.far_field_node_count << "," << d.coarse_sample_count << ","
      << d.exact_node_ratio << "," << d.predicted_node_ratio << ","
      << d.exact_sample_reduction_ratio << "," << d.distance_query_count
      << "," << d.sign_query_count << "," << d.sign_query_reduction_ratio
      << "," << q.contact_band_max_abs_error << ","
      << q.contact_band_rms_error << "," << q.contact_band_p95_error << ","
      << q.contact_band_sign_mismatch_count << ","
      << q.near_surface_sign_mismatch_count << ","
      << q.mean_normal_angle_error_deg << ","
      << q.p95_normal_angle_error_deg << ","
      << q.max_normal_angle_error_deg << "," << q.normal_flip_count << ","
      << q.near_surface_normal_flip_count << ","
      << (q.contact_band_quality_passed ? "true" : "false") << ","
      << (result.effective_speedup_claim_allowed ? "true" : "false");
  return out.str();
}

std::string ContactBandReportWriter::toMarkdown(
    const ContactBandBenchmarkResult& result) {
  std::ostringstream out;
  const ContactBandDiagnostics& d = result.diagnostics;
  const ContactBandQualityMetrics& q = result.quality;
  out << "# AdaSDF-CL Contact-Focused Narrow-Band Sampling Benchmark\n\n";
  out << "- Case id: " << result.case_id << "\n";
  out << "- Success: " << yesNo(result.success) << "\n";
  if (!result.error_message.empty()) {
    out << "- Error: " << result.error_message << "\n";
  }
  out << "- Exact reference time ms: " << result.exact_reference_time_ms << "\n";
  out << "- Contact-band time ms: " << result.contact_band_time_ms << "\n";
  out << "- Speedup: " << result.speedup << "\n";
  out << "- Effective speedup claim allowed: "
      << yesNo(result.effective_speedup_claim_allowed) << "\n\n";
  out << "## Sampling\n\n";
  out << "- Total blocks: " << d.total_block_count << "\n";
  out << "- Contact-band blocks: " << d.contact_band_block_count << "\n";
  out << "- Far-field blocks: " << d.far_field_block_count << "\n";
  out << "- Total nodes: " << d.total_node_count << "\n";
  out << "- Exact nodes: " << d.exact_node_count << "\n";
  out << "- Predicted nodes: " << d.predicted_node_count << "\n";
  out << "- Far-field nodes: " << d.far_field_node_count << "\n";
  out << "- Coarse samples: " << d.coarse_sample_count << "\n";
  out << "- Exact node ratio: " << d.exact_node_ratio << "\n";
  out << "- Exact sample reduction ratio: "
      << d.exact_sample_reduction_ratio << "\n";
  out << "- Distance query count: " << d.distance_query_count << "\n";
  out << "- Sign query count: " << d.sign_query_count << "\n";
  out << "- Sign query reduction ratio: "
      << d.sign_query_reduction_ratio << "\n\n";
  out << "## Contact-Band Quality\n\n";
  out << "- Contact-band check count: " << q.contact_band_check_count << "\n";
  out << "- Contact-band max abs error: "
      << q.contact_band_max_abs_error << "\n";
  out << "- Contact-band RMS error: " << q.contact_band_rms_error << "\n";
  out << "- Contact-band P95 error: " << q.contact_band_p95_error << "\n";
  out << "- Contact-band sign mismatches: "
      << q.contact_band_sign_mismatch_count << "\n";
  out << "- Near-surface sign mismatches: "
      << q.near_surface_sign_mismatch_count << "\n";
  out << "- Mean normal angle error deg: "
      << q.mean_normal_angle_error_deg << "\n";
  out << "- P95 normal angle error deg: "
      << q.p95_normal_angle_error_deg << "\n";
  out << "- Max normal angle error deg: "
      << q.max_normal_angle_error_deg << "\n";
  out << "- Normal flips: " << q.normal_flip_count << "\n";
  out << "- Near-surface normal flips: "
      << q.near_surface_normal_flip_count << "\n";
  out << "- Contact-band quality passed: "
      << yesNo(q.contact_band_quality_passed) << "\n";
  return out.str();
}

void ContactBandReportWriter::writeCsv(
    const std::string& path_string,
    const ContactBandBenchmarkResult& result) {
  if (path_string.empty()) {
    return;
  }
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream out(path);
  out << csvHeader() << "\n" << csvRow(result) << "\n";
}

void ContactBandReportWriter::writeMarkdown(
    const std::string& path_string,
    const ContactBandBenchmarkResult& result) {
  if (path_string.empty()) {
    return;
  }
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream out(path);
  out << toMarkdown(result);
}

}  // namespace adasdf

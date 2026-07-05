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
         "effective_speedup_including_marker,"
         "effective_speedup_excluding_marker,"
         "timing_mode,exact_reference_wall_time_ms,"
         "contact_band_wall_time_ms,speedup_end_to_end,"
         "exact_reference_core_build_time_ms,"
         "contact_band_core_build_time_ms,speedup_core_build,"
         "contact_band_marker_time_ms,contact_band_sampling_time_ms,"
         "contact_band_interpolation_time_ms,contact_band_audit_time_ms,"
         "contact_band_report_time_ms,contact_band_diagnostics_time_ms,"
         "marker_time_fraction_of_wall,audit_time_fraction_of_wall,"
         "diagnostics_time_fraction_of_wall,speedup_excluding_audit,"
         "speedup_excluding_diagnostics,speedup_excluding_marker,"
         "marker_mode,candidate_triangle_aabb_overlap_count,"
         "candidate_cell_count,candidate_triangle_count,"
         "refined_candidate_count,rejected_candidate_count,"
         "accepted_contact_cell_count,marker_false_positive_proxy,"
         "distance_refined_cell_count,distance_rejected_cell_count,"
         "marked_cell_count,marked_node_count,local_halo_node_count,"
         "global_halo_node_count,overmark_ratio_estimate,"
         "marker_time_ms,marker_time_fraction,"
         "triangle_bvh_query_time_ms,box_triangle_distance_time_ms,"
         "marker_refinement_time_ms,distance_refinement_time_ms,"
         "total_block_count,contact_band_block_count,far_field_block_count,"
         "contact_band_block_ratio,"
         "total_node_count,exact_node_count,predicted_node_count,"
         "far_field_node_count,coarse_sample_count,exact_node_ratio,"
         "predicted_node_ratio,exact_sample_reduction_ratio,"
         "distance_query_count,sign_query_count,sign_query_reduction_ratio,"
         "contact_band_max_abs_error,contact_band_rms_error,"
         "contact_band_p95_error,contact_band_sign_mismatch_count,"
         "near_surface_sign_mismatch_count,mean_normal_angle_error_deg,"
         "p95_normal_angle_error_deg,max_normal_angle_error_deg,"
         "normal_flip_count,near_surface_normal_flip_count,"
         "coverage_passed,contact_band_coverage_check_count,"
         "missed_contact_band_point_count,missed_contact_band_cell_count,"
         "contact_band_quality_passed,effective_speedup_claim_allowed,"
         "performance_claim_allowed";
}

std::string ContactBandReportWriter::csvRow(
    const ContactBandBenchmarkResult& result) {
  std::ostringstream out;
  const ContactBandDiagnostics& d = result.diagnostics;
  const ContactBandQualityMetrics& q = result.quality;
  out << result.case_id << "," << result.exact_reference_time_ms << ","
      << result.contact_band_time_ms << "," << result.speedup << ","
      << result.effective_speedup_including_marker << ","
      << result.effective_speedup_excluding_marker << ","
      << result.timing_mode << ","
      << result.exact_reference_wall_time_ms << ","
      << result.contact_band_wall_time_ms << ","
      << result.speedup_end_to_end << ","
      << result.exact_reference_core_build_time_ms << ","
      << result.contact_band_core_build_time_ms << ","
      << result.speedup_core_build << ","
      << d.contact_band_marker_time_ms << ","
      << d.contact_band_sampling_time_ms << ","
      << d.contact_band_interpolation_time_ms << ","
      << d.contact_band_audit_time_ms << ","
      << d.contact_band_report_time_ms << ","
      << d.contact_band_diagnostics_time_ms << ","
      << result.marker_time_fraction_of_wall << ","
      << result.audit_time_fraction_of_wall << ","
      << result.diagnostics_time_fraction_of_wall << ","
      << result.speedup_excluding_audit << ","
      << result.speedup_excluding_diagnostics << ","
      << result.speedup_excluding_marker << ","
      << d.marker_mode << ","
      << d.candidate_triangle_aabb_overlap_count << ","
      << d.candidate_cell_count << "," << d.candidate_triangle_count << ","
      << d.refined_candidate_count << "," << d.rejected_candidate_count
      << "," << d.accepted_contact_cell_count << ","
      << d.marker_false_positive_proxy << ","
      << d.distance_refined_cell_count << ","
      << d.distance_rejected_cell_count << "," << d.marked_cell_count << ","
      << d.marked_node_count << "," << d.local_halo_node_count << ","
      << d.global_halo_node_count << "," << d.overmark_ratio_estimate << ","
      << d.marker_time_ms << "," << d.marker_time_fraction << ","
      << d.triangle_bvh_query_time_ms << ","
      << d.box_triangle_distance_time_ms << ","
      << d.marker_refinement_time_ms << ","
      << d.distance_refinement_time_ms << ","
      << d.total_block_count << "," << d.contact_band_block_count << ","
      << d.far_field_block_count << "," << d.contact_band_block_ratio << ","
      << d.total_node_count << ","
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
      << (q.coverage_passed ? "true" : "false") << ","
      << q.contact_band_coverage_check_count << ","
      << q.missed_contact_band_point_count << ","
      << q.missed_contact_band_cell_count << ","
      << (q.contact_band_quality_passed ? "true" : "false") << ","
      << (result.effective_speedup_claim_allowed ? "true" : "false") << ","
      << (result.performance_claim_allowed ? "true" : "false");
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
  out << "- Timing mode: " << result.timing_mode << "\n";
  out << "- Include audit in wall time: "
      << yesNo(result.include_audit_in_wall_time) << "\n";
  out << "- Include marker in speedup: "
      << yesNo(result.include_marker_in_speedup) << "\n";
  out << "- Exclude audit from speedup: "
      << yesNo(result.exclude_audit_from_speedup) << "\n";
  out << "- Exclude marker from speedup: "
      << yesNo(result.exclude_marker_from_speedup) << "\n";
  out << "- Exact reference wall time ms: "
      << result.exact_reference_wall_time_ms << "\n";
  out << "- Contact-band wall time ms: "
      << result.contact_band_wall_time_ms << "\n";
  out << "- Speedup end-to-end: " << result.speedup_end_to_end << "\n";
  out << "- Exact reference core build time ms: "
      << result.exact_reference_core_build_time_ms << "\n";
  out << "- Contact-band core build time ms: "
      << result.contact_band_core_build_time_ms << "\n";
  out << "- Speedup core build: " << result.speedup_core_build << "\n";
  out << "- Effective speedup including marker: "
      << result.effective_speedup_including_marker << "\n";
  out << "- Effective speedup excluding marker: "
      << result.effective_speedup_excluding_marker << "\n";
  out << "- Speedup excluding audit: "
      << result.speedup_excluding_audit << "\n";
  out << "- Speedup excluding diagnostics: "
      << result.speedup_excluding_diagnostics << "\n";
  out << "- Speedup excluding marker: "
      << result.speedup_excluding_marker << "\n";
  out << "- Marker mode: " << d.marker_mode << "\n";
  out << "- Effective speedup claim allowed: "
      << yesNo(result.effective_speedup_claim_allowed) << "\n\n";
  out << "- Performance claim allowed: "
      << yesNo(result.performance_claim_allowed) << "\n\n";
  out << "## Sampling\n\n";
  out << "- Total blocks: " << d.total_block_count << "\n";
  out << "- Contact-band blocks: " << d.contact_band_block_count << "\n";
  out << "- Far-field blocks: " << d.far_field_block_count << "\n";
  out << "- Contact-band block ratio: " << d.contact_band_block_ratio << "\n";
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
  out << "## Marker Diagnostics\n\n";
  out << "- Candidate triangle AABB overlaps: "
      << d.candidate_triangle_aabb_overlap_count << "\n";
  out << "- Candidate cells: " << d.candidate_cell_count << "\n";
  out << "- Candidate triangles: " << d.candidate_triangle_count << "\n";
  out << "- Refined candidates: " << d.refined_candidate_count << "\n";
  out << "- Rejected candidates: " << d.rejected_candidate_count << "\n";
  out << "- Accepted contact cells: "
      << d.accepted_contact_cell_count << "\n";
  out << "- Marker false-positive proxy: "
      << d.marker_false_positive_proxy << "\n";
  out << "- Distance-refined cells: "
      << d.distance_refined_cell_count << "\n";
  out << "- Distance-rejected cells: "
      << d.distance_rejected_cell_count << "\n";
  out << "- Marked cells: " << d.marked_cell_count << "\n";
  out << "- Marked nodes: " << d.marked_node_count << "\n";
  out << "- Local halo nodes: " << d.local_halo_node_count << "\n";
  out << "- Global halo nodes: " << d.global_halo_node_count << "\n";
  out << "- Overmark ratio estimate: "
      << d.overmark_ratio_estimate << "\n";
  out << "- Marker time ms: " << d.marker_time_ms << "\n";
  out << "- Marker time fraction: " << d.marker_time_fraction << "\n";
  out << "- Contact-band marker time ms: "
      << d.contact_band_marker_time_ms << "\n";
  out << "- Contact-band sampling time ms: "
      << d.contact_band_sampling_time_ms << "\n";
  out << "- Contact-band interpolation time ms: "
      << d.contact_band_interpolation_time_ms << "\n";
  out << "- Contact-band audit time ms: "
      << d.contact_band_audit_time_ms << "\n";
  out << "- Contact-band report time ms: "
      << d.contact_band_report_time_ms << "\n";
  out << "- Contact-band diagnostics time ms: "
      << d.contact_band_diagnostics_time_ms << "\n";
  out << "- Marker time fraction of wall: "
      << result.marker_time_fraction_of_wall << "\n";
  out << "- Audit time fraction of wall: "
      << result.audit_time_fraction_of_wall << "\n";
  out << "- Diagnostics time fraction of wall: "
      << result.diagnostics_time_fraction_of_wall << "\n";
  out << "- Triangle BVH query time ms: "
      << d.triangle_bvh_query_time_ms << "\n";
  out << "- Box-triangle distance time ms: "
      << d.box_triangle_distance_time_ms << "\n";
  out << "- Marker refinement time ms: "
      << d.marker_refinement_time_ms << "\n";
  out << "- Distance refinement time ms: "
      << d.distance_refinement_time_ms << "\n\n";
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
  out << "- Coverage passed: " << yesNo(q.coverage_passed) << "\n";
  out << "- Coverage check count: "
      << q.contact_band_coverage_check_count << "\n";
  out << "- Missed contact-band points: "
      << q.missed_contact_band_point_count << "\n";
  out << "- Missed contact-band cells: "
      << q.missed_contact_band_cell_count << "\n";
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

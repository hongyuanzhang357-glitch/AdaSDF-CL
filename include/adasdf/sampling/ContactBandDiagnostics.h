#pragma once

#include <cstddef>
#include <string>

namespace adasdf {

struct ContactBandDiagnostics {
  std::string marker_mode = "conservative-aabb";
  std::size_t total_block_count = 0;
  std::size_t contact_band_block_count = 0;
  std::size_t far_field_block_count = 0;
  double contact_band_block_ratio = 0.0;

  std::size_t total_node_count = 0;
  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t far_field_node_count = 0;
  std::size_t coarse_sample_count = 0;

  std::size_t distance_query_count = 0;
  std::size_t sign_query_count = 0;

  std::size_t candidate_triangle_aabb_overlap_count = 0;
  std::size_t distance_refined_cell_count = 0;
  std::size_t distance_rejected_cell_count = 0;
  std::size_t marked_cell_count = 0;
  std::size_t marked_node_count = 0;
  std::size_t local_halo_node_count = 0;
  std::size_t global_halo_node_count = 0;
  double overmark_ratio_estimate = 0.0;

  double exact_sampling_time_ms = 0.0;
  double coarse_sampling_time_ms = 0.0;
  double interpolation_time_ms = 0.0;
  double total_time_ms = 0.0;
  double marker_time_ms = 0.0;
  double distance_refinement_time_ms = 0.0;

  double exact_node_ratio = 0.0;
  double predicted_node_ratio = 0.0;
  double exact_sample_reduction_ratio = 0.0;
  double sign_query_reduction_ratio = 0.0;
};

void finalizeContactBandDiagnostics(ContactBandDiagnostics* diagnostics);

}  // namespace adasdf

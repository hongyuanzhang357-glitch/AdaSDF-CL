#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace adasdf {

struct NarrowBandBrickBuildStats {
  bool success = false;
  std::string error_message;

  std::size_t sampling_node_count = 0;
  std::map<int, std::size_t> sampling_node_count_by_level;
  std::map<int, std::size_t> exact_sample_estimate_by_level;
  std::map<int, std::size_t> interpolated_sample_estimate_by_level;
  std::map<int, std::size_t> contact_band_node_count_by_level;
  std::map<int, std::size_t> far_field_node_count_by_level;

  std::size_t brick_count = 0;
  std::map<int, std::size_t> brick_count_by_level;
  std::map<std::string, std::size_t> tensor_dim_distribution;
  std::size_t total_tensor_nodes = 0;
  std::size_t total_exact_source_nodes = 0;
  std::size_t total_interpolated_fill_nodes = 0;
  std::size_t estimated_compressed_bytes = 0;
  std::size_t estimated_expanded_bytes = 0;
  std::size_t estimated_active_expanded_bytes = 0;
  std::size_t max_single_brick_expanded_bytes = 0;
  std::size_t brick_split_recommendation_count = 0;
  std::size_t brick_merge_recommendation_count = 0;

  std::size_t compressed_block_count = 0;
  std::size_t dense_fallback_block_count = 0;
  int rank_min = 0;
  double rank_mean = 0.0;
  int rank_max = 0;
  std::size_t contact_band_sign_flip_count = 0;
  double contact_band_p95_compression_error = 0.0;

  bool sign_protected_fill_enabled = false;
  std::size_t sign_check_node_count = 0;
  std::size_t sign_check_mismatch_count = 0;
  std::size_t fill_sign_check_mismatch_count = 0;
  std::size_t fill_fallback_exact_node_count = 0;
  std::size_t zero_crossing_cell_count = 0;
  std::size_t zero_crossing_risk_cell_count = 0;
  std::size_t protected_zero_crossing_cell_count = 0;

  double mesh_load_time_ms = 0.0;
  double bvh_build_time_ms = 0.0;
  double sampling_tree_time_ms = 0.0;
  double brick_planning_time_ms = 0.0;
  double tensor_generation_time_ms = 0.0;
  double compression_time_ms = 0.0;
  double write_time_ms = 0.0;
  double total_time_ms = 0.0;

  std::vector<std::string> warnings;
};

}  // namespace adasdf

#pragma once

#include <cstddef>

#include "adasdf/generation/AdaptiveTreeStats.h"

namespace adasdf {

struct BuildProfileCounters {
  std::size_t num_vertices = 0;
  std::size_t num_triangles = 0;
  std::size_t num_grid_points = 0;
  std::size_t num_blocks = 0;
  std::size_t num_adaptive_tree_nodes = 0;
  std::size_t num_adaptive_leaf_blocks = 0;
  std::size_t uniform_max_level_leaf_count = 0;
  std::size_t total_logical_node_count = 0;
  std::size_t uniform_max_level_logical_node_count = 0;
  double adaptive_tree_sparsity_ratio = 0.0;
  std::size_t num_contact_band_blocks = 0;
  std::size_t num_distance_queries = 0;
  std::size_t num_sign_queries = 0;
  std::size_t num_triangle_tests_total = 0;
  double avg_triangles_tested_per_query = 0.0;
  std::size_t bvh_node_visit_count = 0;
  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t fallback_count = 0;
  std::size_t compressed_block_count = 0;
  std::size_t dense_fallback_block_count = 0;
  std::size_t output_bytes = 0;
  bool compression_guard_enabled = false;
  std::size_t guarded_block_count = 0;
  std::size_t kept_dense_due_to_sign_count = 0;
  std::size_t kept_dense_due_to_error_count = 0;
  std::size_t near_zero_compression_sign_flip_count = 0;
  double near_zero_compression_p95_error = 0.0;
  std::size_t dense_fallback_memory_bytes = 0;
  std::size_t compressed_memory_bytes_after_guard = 0;

  bool has_adaptive_tree_stats = false;
  AdaptiveTreeStats adaptive_tree;

  bool sample_cache_enabled = false;
  const char* sample_cache_scope = "off";
  std::size_t sample_cache_entries = 0;
  std::size_t sample_cache_hits = 0;
  std::size_t sample_cache_misses = 0;
  double sample_cache_hit_rate = 0.0;
  std::size_t distance_cache_hits = 0;
  std::size_t distance_cache_misses = 0;
  std::size_t sign_cache_hits = 0;
  std::size_t sign_cache_misses = 0;
  std::size_t corner_cache_hits = 0;
  std::size_t corner_cache_misses = 0;
  std::size_t block_point_duplicate_count = 0;
  std::size_t marker_candidate_cache_hits = 0;
  std::size_t marker_candidate_cache_misses = 0;
  std::size_t marker_decision_cache_hits = 0;
  std::size_t marker_decision_cache_misses = 0;
  std::size_t distance_queries_saved = 0;
  std::size_t sign_queries_saved = 0;
  std::size_t box_triangle_distance_saved = 0;
  std::size_t cache_memory_estimate_bytes = 0;
};

}  // namespace adasdf

#pragma once

#include <cstddef>

namespace adasdf {

struct BuildProfileCounters {
  std::size_t num_vertices = 0;
  std::size_t num_triangles = 0;
  std::size_t num_grid_points = 0;
  std::size_t num_blocks = 0;
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

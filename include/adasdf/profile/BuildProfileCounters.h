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
};

}  // namespace adasdf

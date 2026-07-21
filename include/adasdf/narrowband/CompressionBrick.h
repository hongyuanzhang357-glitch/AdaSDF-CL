#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct CompressionBrick {
  int brick_id = -1;
  int brick_level = 0;
  AABB bounds;
  std::vector<int> covered_sampling_node_ids;
  int finest_sampling_level_inside = 0;
  int coarsest_sampling_level_inside = 0;
  std::size_t contact_band_node_count = 0;
  std::size_t far_field_node_count = 0;
  int tensor_nx = 0;
  int tensor_ny = 0;
  int tensor_nz = 0;
  std::size_t tensor_node_count = 0;
  std::size_t exact_source_node_count = 0;
  std::size_t interpolated_fill_node_count = 0;
  std::size_t sign_check_node_count = 0;
  std::size_t sign_check_mismatch_count = 0;
  std::size_t fill_fallback_exact_node_count = 0;
  std::size_t zero_crossing_cell_count = 0;
  std::size_t protected_zero_crossing_cell_count = 0;
  int estimated_rank = 0;
  std::size_t estimated_compressed_bytes = 0;
  std::size_t estimated_expanded_bytes = 0;
  bool should_split = false;
  bool should_merge = false;
  std::string split_or_merge_reason = "none";
  std::string split_reason = "deterministic_grid";
  std::string merge_reason = "none";
};

}  // namespace adasdf

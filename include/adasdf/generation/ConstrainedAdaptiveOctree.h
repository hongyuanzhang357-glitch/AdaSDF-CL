#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "adasdf/acceleration/ExactSDFOracle.h"

namespace adasdf {

struct ConstrainedAdaptiveOctreeOptions {
  int min_depth = 1;
  int max_depth = 6;
  double padding_fraction = 0.05;
  double min_cell_size = 0.0;
  double narrow_band_distance = 0.01;
  double max_center_interpolation_residual = 0.001;
  std::size_t max_overlapping_triangles = 16;
  double max_normal_complexity = 0.15;
  std::size_t max_node_count = 1000000;
};

struct ConstrainedAdaptiveOctreeNode {
  int node_id = -1;
  int parent_id = -1;
  int level = 0;
  AABB bounds;
  bool is_leaf = true;
  std::array<int, 8> child_ids{{-1, -1, -1, -1, -1, -1, -1, -1}};
  std::array<double, 8> corner_phi{{0.0, 0.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 0.0}};
  double center_phi = 0.0;
  double center_interpolated_phi = 0.0;
  double interpolation_residual = 0.0;
  double min_abs_phi = 0.0;
  std::size_t overlapping_triangle_count = 0;
  double normal_complexity = 0.0;
  bool sign_change = false;
  bool near_surface = false;
  bool triangle_overlap = false;
  bool geometry_complex = false;
};

struct ConstrainedAdaptiveOctreeStats {
  bool success = false;
  std::string error_message;
  std::size_t node_count = 0;
  std::size_t leaf_count = 0;
  std::size_t near_surface_leaf_count = 0;
  std::size_t sign_change_leaf_count = 0;
  std::size_t triangle_overlap_leaf_count = 0;
  std::size_t residual_refinement_count = 0;
  std::size_t triangle_refinement_count = 0;
  std::size_t complexity_refinement_count = 0;
  std::map<int, std::size_t> node_count_by_level;
  int max_depth_used = 0;
  std::size_t exact_query_requests = 0;
  std::size_t unique_exact_node_count = 0;
  std::size_t exact_cache_hit_count = 0;
  std::size_t uniform_finest_grid_node_count = 0;
  std::size_t bvh_node_visits = 0;
  std::size_t triangle_tests = 0;
  double build_time_ms = 0.0;
};

struct ConstrainedAdaptiveOctree {
  AABB bounds;
  int root_id = -1;
  std::vector<ConstrainedAdaptiveOctreeNode> nodes;

  std::vector<int> leafNodeIds() const;
};

class ConstrainedAdaptiveOctreeBuilder {
 public:
  static ConstrainedAdaptiveOctree build(
      const TriangleMesh& mesh,
      ExactSDFOracle* oracle,
      const ConstrainedAdaptiveOctreeOptions& options,
      ConstrainedAdaptiveOctreeStats* stats = nullptr);
};

}  // namespace adasdf

#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/geometry/Transform.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

namespace adasdf {

struct SamplingOctreeNode {
  int node_id = -1;
  int level = 0;
  AABB bounds;
  int parent_id = -1;
  std::vector<int> child_ids;
  bool near_zero_surface = false;
  bool contact_band = false;
  bool far_field = true;
  bool zero_crossing_risk = false;
  bool high_curvature_hint = false;
  bool small_gap_hint = false;
  double target_sample_spacing = 0.0;
  bool exact_sample_required = false;
  bool exact_required = false;
  bool can_be_interpolated = true;
  std::size_t estimated_exact_sample_count = 0;
  std::size_t estimated_interpolated_sample_count = 0;
};

struct SamplingOctreePlan {
  AABB bounds;
  std::vector<SamplingOctreeNode> nodes;
  std::map<int, std::size_t> node_count_by_level;
  std::map<int, std::size_t> exact_sample_estimate_by_level;
  std::map<int, std::size_t> interpolated_sample_estimate_by_level;
  std::map<int, std::size_t> contact_band_node_count_by_level;
  std::map<int, std::size_t> far_field_node_count_by_level;
};

class SamplingOctree {
 public:
  static SamplingOctreePlan build(
      const TriangleMesh& mesh,
      const BVHSDFSampler& sampler,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

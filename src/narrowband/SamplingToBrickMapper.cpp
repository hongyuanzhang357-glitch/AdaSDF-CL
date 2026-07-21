#include "adasdf/narrowband/SamplingToBrickMapper.h"

#include <algorithm>
#include <limits>

namespace adasdf {
namespace {

Vector3 center(const AABB& box) {
  return {
      0.5 * (box.min.x + box.max.x),
      0.5 * (box.min.y + box.max.y),
      0.5 * (box.min.z + box.max.z)};
}

bool contains(const AABB& box, const Vector3& p) {
  const double eps = 1.0e-12;
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

}  // namespace

void SamplingToBrickMapper::map(
    const SamplingOctreePlan& sampling_plan,
    CompressionBrickTreePlan* brick_plan) {
  if (brick_plan == nullptr) {
    return;
  }
  for (CompressionBrick& brick : brick_plan->bricks) {
    brick.coarsest_sampling_level_inside = std::numeric_limits<int>::max();
  }
  for (const SamplingOctreeNode& node : sampling_plan.nodes) {
    const Vector3 p = center(node.bounds);
    for (CompressionBrick& brick : brick_plan->bricks) {
      if (!contains(brick.bounds, p)) {
        continue;
      }
      brick.covered_sampling_node_ids.push_back(node.node_id);
      brick.finest_sampling_level_inside =
          std::max(brick.finest_sampling_level_inside, node.level);
      brick.coarsest_sampling_level_inside =
          std::min(brick.coarsest_sampling_level_inside, node.level);
      if (node.contact_band || node.near_zero_surface) {
        ++brick.contact_band_node_count;
      } else {
        ++brick.far_field_node_count;
      }
      break;
    }
  }
  for (CompressionBrick& brick : brick_plan->bricks) {
    if (brick.coarsest_sampling_level_inside == std::numeric_limits<int>::max()) {
      brick.coarsest_sampling_level_inside = 0;
    }
  }
}

}  // namespace adasdf

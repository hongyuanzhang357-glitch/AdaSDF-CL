#include "adasdf/narrowband/CompressionBrickTree.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "adasdf/narrowband/BrickTensorDimensionPolicy.h"
#include "adasdf/narrowband/SamplingToBrickMapper.h"

namespace adasdf {
namespace {

std::string dimKey(int n) {
  std::ostringstream out;
  out << n << "x" << n << "x" << n;
  return out.str();
}

Vector3 lerpBox(const AABB& box, double u, double v, double w) {
  return {
      box.min.x + (box.max.x - box.min.x) * u,
      box.min.y + (box.max.y - box.min.y) * v,
      box.min.z + (box.max.z - box.min.z) * w};
}

}  // namespace

CompressionBrickTreePlan CompressionBrickTree::build(
    const SamplingOctreePlan& sampling_plan,
    const NarrowBandBrickBuildOptions& options) {
  CompressionBrickTreePlan plan;
  plan.bounds = sampling_plan.bounds;
  const int brick_level =
      brickLevelForSamplingLevel(options.brick_level_map,
                                 options.max_sampling_level);
  const int axis_count = 1 << std::max(0, brick_level);
  int brick_id = 0;
  for (int z = 0; z < axis_count; ++z) {
    for (int y = 0; y < axis_count; ++y) {
      for (int x = 0; x < axis_count; ++x) {
        CompressionBrick brick;
        brick.brick_id = brick_id++;
        brick.brick_level = brick_level;
        const double inv = 1.0 / static_cast<double>(axis_count);
        brick.bounds.valid = true;
        brick.bounds.min =
            lerpBox(plan.bounds, x * inv, y * inv, z * inv);
        brick.bounds.max =
            lerpBox(plan.bounds, (x + 1) * inv, (y + 1) * inv, (z + 1) * inv);
        brick.estimated_rank = std::max(1, options.max_rank);
        plan.bricks.push_back(std::move(brick));
      }
    }
  }
  SamplingToBrickMapper::map(sampling_plan, &plan);
  for (CompressionBrick& brick : plan.bricks) {
    const BrickTensorDimensionDecision decision =
        BrickTensorDimensionPolicy::decide(brick, options);
    brick.tensor_nx = decision.tensor_nx;
    brick.tensor_ny = decision.tensor_ny;
    brick.tensor_nz = decision.tensor_nz;
    brick.tensor_node_count = decision.tensor_node_count;
    brick.estimated_expanded_bytes = decision.estimated_expanded_bytes;
    brick.estimated_compressed_bytes = decision.estimated_expanded_bytes;
    brick.should_split = decision.should_split;
    brick.should_merge = decision.should_merge;
    brick.split_or_merge_reason = decision.reason;
    brick.split_reason = decision.should_split ? decision.reason : "none";
    brick.merge_reason = decision.should_merge ? decision.reason : "none";
    ++plan.brick_count_by_level[brick.brick_level];
    ++plan.tensor_dim_distribution[dimKey(brick.tensor_nx)];
  }
  return plan;
}

}  // namespace adasdf

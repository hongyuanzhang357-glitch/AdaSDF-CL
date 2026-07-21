#pragma once

#include <map>
#include <vector>

#include "adasdf/narrowband/CompressionBrick.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"
#include "adasdf/narrowband/SamplingOctree.h"

namespace adasdf {

struct CompressionBrickTreePlan {
  AABB bounds;
  std::vector<CompressionBrick> bricks;
  std::map<int, std::size_t> brick_count_by_level;
  std::map<std::string, std::size_t> tensor_dim_distribution;
};

class CompressionBrickTree {
 public:
  static CompressionBrickTreePlan build(
      const SamplingOctreePlan& sampling_plan,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

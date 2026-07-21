#pragma once

#include "adasdf/narrowband/CompressionBrickTree.h"
#include "adasdf/narrowband/SamplingOctree.h"

namespace adasdf {

class SamplingToBrickMapper {
 public:
  static void map(
      const SamplingOctreePlan& sampling_plan,
      CompressionBrickTreePlan* brick_plan);
};

}  // namespace adasdf

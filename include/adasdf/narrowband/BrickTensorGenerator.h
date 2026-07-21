#pragma once

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/narrowband/CompressionBrick.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

namespace adasdf {

struct BrickTensorResult {
  CompressionBrick brick;
  AdaptiveSDFBlock block;
};

class BrickTensorGenerator {
 public:
  static BrickTensorResult generate(
      const CompressionBrick& brick,
      const BVHSDFSampler& sampler,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

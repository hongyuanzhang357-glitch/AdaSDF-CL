#pragma once

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

namespace adasdf {

struct BrickTensorFillDecision {
  bool exact_source = true;
  bool interpolated_fill = false;
  double phi = 0.0;
};

class BrickTensorFillPolicy {
 public:
  static BrickTensorFillDecision evaluate(
      const Vector3& point,
      const BVHSDFSampler& sampler,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

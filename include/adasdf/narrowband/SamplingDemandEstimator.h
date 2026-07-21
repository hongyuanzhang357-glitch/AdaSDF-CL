#pragma once

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/geometry/Transform.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

namespace adasdf {

struct SamplingDemandEstimate {
  bool near_zero_surface = false;
  bool contact_band = false;
  bool far_field = true;
  bool zero_crossing_risk = false;
  bool exact_required = false;
  bool should_refine = false;
  double min_abs_phi = 0.0;
  double target_sample_spacing = 0.0;
  std::size_t estimated_exact_sample_count = 0;
  std::size_t estimated_interpolated_sample_count = 0;
};

class SamplingDemandEstimator {
 public:
  static SamplingDemandEstimate estimate(
      const AABB& bounds,
      int level,
      const BVHSDFSampler& sampler,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

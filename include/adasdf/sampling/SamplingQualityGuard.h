#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/sampling/HierarchicalSDFPredictor.h"

namespace adasdf {

struct SamplingQualityOptions {
  int check_samples_per_axis = 3;

  double max_abs_error_limit = 1e-3;
  double rms_error_limit = 5e-4;
  double p95_error_limit = 1e-3;

  double sign_epsilon = 1e-12;

  bool reject_on_sign_mismatch = true;
  bool reject_on_near_surface_sign_mismatch = true;

  double near_surface_band = 1e-3;
};

struct SamplingQualityReport {
  bool accepted = false;

  std::size_t check_sample_count = 0;

  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_abs_error = 0.0;

  std::size_t sign_mismatch_count = 0;
  std::size_t near_surface_sign_mismatch_count = 0;

  std::vector<std::string> warnings;
};

class SamplingQualityGuard {
 public:
  static SamplingQualityReport check(
      const AABB& block_bounds,
      int nx,
      int ny,
      int nz,
      const std::vector<double>& predicted_phi,
      BVHSDFSampler& exact_sampler,
      const SamplingQualityOptions& options);
};

}  // namespace adasdf

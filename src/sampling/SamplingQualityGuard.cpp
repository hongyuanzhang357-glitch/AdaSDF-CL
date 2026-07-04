#include "adasdf/sampling/SamplingQualityGuard.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace adasdf {
namespace {

Vector3 lerpPoint(const AABB& bounds, double tx, double ty, double tz) {
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * tx,
      bounds.min.y + (bounds.max.y - bounds.min.y) * ty,
      bounds.min.z + (bounds.max.z - bounds.min.z) * tz};
}

int signBucket(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

}  // namespace

SamplingQualityReport SamplingQualityGuard::check(
    const AABB& block_bounds,
    int nx,
    int ny,
    int nz,
    const std::vector<double>& predicted_phi,
    BVHSDFSampler& exact_sampler,
    const SamplingQualityOptions& options) {
  SamplingQualityReport report;
  if (!block_bounds.valid || nx < 2 || ny < 2 || nz < 2 ||
      predicted_phi.size() !=
          static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
              static_cast<std::size_t>(nz)) {
    report.warnings.push_back("invalid predicted grid for quality guard");
    return report;
  }
  const int samples = std::max(2, options.check_samples_per_axis);
  std::vector<double> abs_errors;
  abs_errors.reserve(
      static_cast<std::size_t>(samples) * static_cast<std::size_t>(samples) *
      static_cast<std::size_t>(samples));
  double sum_abs = 0.0;
  double sum_sq = 0.0;

  for (int k = 0; k < samples; ++k) {
    const double tz = samples == 1 ? 0.0 : static_cast<double>(k) /
                                             static_cast<double>(samples - 1);
    for (int j = 0; j < samples; ++j) {
      const double ty = samples == 1 ? 0.0 : static_cast<double>(j) /
                                               static_cast<double>(samples - 1);
      for (int i = 0; i < samples; ++i) {
        const double tx = samples == 1 ? 0.0 : static_cast<double>(i) /
                                                 static_cast<double>(samples - 1);
        const Vector3 point = lerpPoint(block_bounds, tx, ty, tz);
        const double predicted = interpolateGridPhi(
            block_bounds, nx, ny, nz, predicted_phi, point);
        const BVHSDFSampleResult exact = exact_sampler.sample(point);
        const double exact_phi = exact.success ? exact.phi : 0.0;
        const double err = std::abs(predicted - exact_phi);
        if (!std::isfinite(err) || !std::isfinite(exact_phi) ||
            !std::isfinite(predicted)) {
          report.warnings.push_back("non-finite quality guard sample");
          return report;
        }
        abs_errors.push_back(err);
        sum_abs += err;
        sum_sq += err * err;

        const int predicted_sign = signBucket(predicted, options.sign_epsilon);
        const int exact_sign = signBucket(exact_phi, options.sign_epsilon);
        if (predicted_sign != exact_sign) {
          ++report.sign_mismatch_count;
          if (std::min(std::abs(predicted), std::abs(exact_phi)) <=
              options.near_surface_band) {
            ++report.near_surface_sign_mismatch_count;
          }
        }
      }
    }
  }

  report.check_sample_count = abs_errors.size();
  if (abs_errors.empty()) {
    report.warnings.push_back("no quality guard samples were generated");
    return report;
  }
  std::sort(abs_errors.begin(), abs_errors.end());
  report.max_abs_error = abs_errors.back();
  report.mean_abs_error = sum_abs / static_cast<double>(abs_errors.size());
  report.rms_error = std::sqrt(sum_sq / static_cast<double>(abs_errors.size()));
  const std::size_t p95_index = std::min(
      abs_errors.size() - 1,
      static_cast<std::size_t>(
          std::ceil(0.95 * static_cast<double>(abs_errors.size()))) -
          1);
  report.p95_abs_error = abs_errors[p95_index];

  report.accepted =
      report.max_abs_error <= options.max_abs_error_limit &&
      report.rms_error <= options.rms_error_limit &&
      report.p95_abs_error <= options.p95_error_limit;
  if (options.reject_on_sign_mismatch && report.sign_mismatch_count > 0) {
    report.accepted = false;
  }
  if (options.reject_on_near_surface_sign_mismatch &&
      report.near_surface_sign_mismatch_count > 0) {
    report.accepted = false;
  }
  return report;
}

}  // namespace adasdf

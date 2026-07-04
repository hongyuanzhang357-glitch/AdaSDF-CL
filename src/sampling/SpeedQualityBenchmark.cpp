#include "adasdf/sampling/SpeedQualityBenchmark.h"

#include <algorithm>
#include <cmath>

#include "adasdf/mesh/STLReader.h"

namespace adasdf {
namespace {

Vector3 lerpPoint(const AABB& bounds, double tx, double ty, double tz) {
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * tx,
      bounds.min.y + (bounds.max.y - bounds.min.y) * ty,
      bounds.min.z + (bounds.max.z - bounds.min.z) * tz};
}

int signBucket(double value) {
  constexpr double eps = 1e-12;
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

}  // namespace

SpeedQualityBenchmarkResult SpeedQualityBenchmark::run(
    const TriangleMesh& mesh,
    const SpeedQualityBenchmarkOptions& options) {
  SpeedQualityBenchmarkResult result;
  AdaptiveBlockSDFBuildOptions exact_options = options.build_options;
  exact_options.hierarchical_sampling.enable_hierarchical_sampling = false;
  AdaptiveBlockSDFBuildReport exact_report;
  auto exact_model =
      AdaptiveBlockSDFBuilder::fromMesh(mesh, exact_options, &exact_report);
  if (!exact_model) {
    result.error_message = "exact build failed: " + exact_report.error_message;
    return result;
  }

  AdaptiveBlockSDFBuildOptions hierarchical_options = options.build_options;
  hierarchical_options.hierarchical_sampling.enable_hierarchical_sampling = true;
  AdaptiveBlockSDFBuildReport hierarchical_report;
  auto hierarchical_model = AdaptiveBlockSDFBuilder::fromMesh(
      mesh, hierarchical_options, &hierarchical_report);
  if (!hierarchical_model) {
    result.error_message =
        "hierarchical build failed: " + hierarchical_report.error_message;
    return result;
  }

  result.exact_build_time_ms = exact_report.build_time_ms;
  result.hierarchical_build_time_ms = hierarchical_report.build_time_ms;
  if (result.hierarchical_build_time_ms > 0.0) {
    result.speedup =
        result.exact_build_time_ms / result.hierarchical_build_time_ms;
  }
  result.exact_sample_count = exact_report.acceleration_stats.sample_count;
  result.hierarchical_exact_sample_count =
      hierarchical_report.hierarchical_sampling.exact_sample_count;
  result.predicted_sample_count =
      hierarchical_report.hierarchical_sampling.predicted_sample_count;
  result.fallback_block_count =
      hierarchical_report.hierarchical_sampling.fallback_exact_block_count;

  const AABB bounds = exact_model->boundingBox();
  const int samples = std::max(2, options.comparison_samples_per_axis);
  std::vector<double> abs_errors;
  double sum_abs = 0.0;
  double sum_sq = 0.0;
  for (int k = 0; k < samples; ++k) {
    const double tz = static_cast<double>(k) / static_cast<double>(samples - 1);
    for (int j = 0; j < samples; ++j) {
      const double ty = static_cast<double>(j) / static_cast<double>(samples - 1);
      for (int i = 0; i < samples; ++i) {
        const double tx = static_cast<double>(i) / static_cast<double>(samples - 1);
        const Vector3 point = lerpPoint(bounds, tx, ty, tz);
        const double exact_phi = exact_model->sampleDistance(point);
        const double hierarchical_phi = hierarchical_model->sampleDistance(point);
        const double err = std::abs(exact_phi - hierarchical_phi);
        if (!std::isfinite(err)) {
          result.error_message = "non-finite benchmark comparison error";
          return result;
        }
        abs_errors.push_back(err);
        sum_abs += err;
        sum_sq += err * err;
        if (signBucket(exact_phi) != signBucket(hierarchical_phi)) {
          ++result.sign_mismatch_count;
          if (std::min(std::abs(exact_phi), std::abs(hierarchical_phi)) <=
              options.build_options.hierarchical_sampling.near_surface_band) {
            ++result.near_surface_sign_mismatch_count;
          }
        }
      }
    }
  }
  std::sort(abs_errors.begin(), abs_errors.end());
  result.max_abs_error = abs_errors.empty() ? 0.0 : abs_errors.back();
  result.mean_abs_error =
      abs_errors.empty() ? 0.0 : sum_abs / static_cast<double>(abs_errors.size());
  result.rms_error =
      abs_errors.empty() ? 0.0 : std::sqrt(sum_sq / static_cast<double>(abs_errors.size()));
  const std::size_t p95_index = abs_errors.empty()
                                    ? 0
                                    : std::min(
                                          abs_errors.size() - 1,
                                          static_cast<std::size_t>(
                                              std::ceil(0.95 * abs_errors.size())) -
                                              1);
  result.p95_error = abs_errors.empty() ? 0.0 : abs_errors[p95_index];
  result.quality_gate_passed =
      result.max_abs_error <=
          options.build_options.hierarchical_sampling.target_max_abs_error &&
      result.rms_error <=
          options.build_options.hierarchical_sampling.target_rms_error &&
      result.p95_error <=
          options.build_options.hierarchical_sampling.target_p95_error &&
      result.sign_mismatch_count == 0 &&
      result.near_surface_sign_mismatch_count == 0;
  result.success = true;
  return result;
}

SpeedQualityBenchmarkResult SpeedQualityBenchmark::runSTL(
    const std::string& stl_path,
    const SpeedQualityBenchmarkOptions& options) {
  STLReadOptions read_options;
  const STLReadResult read = STLReader::read(stl_path, read_options);
  if (!read.success) {
    SpeedQualityBenchmarkResult result;
    result.error_message = "Failed to read STL: " + read.error_message;
    return result;
  }
  return run(read.mesh, options);
}

}  // namespace adasdf

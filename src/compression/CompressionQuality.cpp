#include "adasdf/compression/CompressionQuality.h"

#include <algorithm>
#include <cmath>

#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {
namespace {

double percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const std::size_t index = static_cast<std::size_t>(
      std::round(std::clamp(q, 0.0, 1.0) * static_cast<double>(values.size() - 1)));
  return values[index];
}

bool signMismatch(double a, double b, double eps) {
  if (std::abs(a) <= eps || std::abs(b) <= eps) {
    return false;
  }
  return (a < 0.0) != (b < 0.0);
}

void addSample(
    const SDFModel& reference,
    const SDFModel& compressed,
    const Vector3& p,
    const CompressionQualityOptions& options,
    CompressionQualityReport* report,
    std::vector<double>* errors,
    double* sum_abs,
    double* sum_sq) {
  const double ref = reference.sampleDistance(p);
  const double cmp = compressed.sampleDistance(p);
  if (!std::isfinite(ref) || !std::isfinite(cmp)) {
    report->warnings.push_back("non-finite sample skipped");
    return;
  }
  const double diff = cmp - ref;
  const double abs_error = std::abs(diff);
  errors->push_back(abs_error);
  report->max_abs_error = std::max(report->max_abs_error, abs_error);
  *sum_abs += abs_error;
  *sum_sq += diff * diff;
  ++report->sample_count;
  if (signMismatch(ref, cmp, options.sign_epsilon)) {
    ++report->sign_mismatch_count;
    if (std::abs(ref) <= options.near_surface_band) {
      ++report->near_surface_sign_mismatch_count;
    }
  }
}

}  // namespace

CompressionQualityReport CompressionQuality::compare(
    const SDFModel& reference_dense_model,
    const SDFModel& compressed_model,
    const CompressionQualityOptions& options) {
  CompressionQualityReport report;
  if (!reference_dense_model.queryBackendAvailable() ||
      !compressed_model.queryBackendAvailable()) {
    report.error_message = "CompressionQuality requires queryable models.";
    return report;
  }

  std::vector<double> errors;
  double sum_abs = 0.0;
  double sum_sq = 0.0;
  const int axis = std::max(2, options.samples_per_block_axis);

  if (const auto* adaptive =
          dynamic_cast<const AdaptiveBlockSDFModel*>(&reference_dense_model)) {
    for (const AdaptiveSDFBlock& block : adaptive->blockSet().blocks) {
      for (int ix = 0; ix < axis; ++ix) {
        for (int iy = 0; iy < axis; ++iy) {
          for (int iz = 0; iz < axis; ++iz) {
            const double tx =
                axis == 1 ? 0.0 : static_cast<double>(ix) / static_cast<double>(axis - 1);
            const double ty =
                axis == 1 ? 0.0 : static_cast<double>(iy) / static_cast<double>(axis - 1);
            const double tz =
                axis == 1 ? 0.0 : static_cast<double>(iz) / static_cast<double>(axis - 1);
            const Vector3 p{
                block.bounds.min.x + tx * (block.bounds.max.x - block.bounds.min.x),
                block.bounds.min.y + ty * (block.bounds.max.y - block.bounds.min.y),
                block.bounds.min.z + tz * (block.bounds.max.z - block.bounds.min.z)};
            addSample(
                reference_dense_model,
                compressed_model,
                p,
                options,
                &report,
                &errors,
                &sum_abs,
                &sum_sq);
          }
        }
      }
    }
  } else {
    const AABB bounds = reference_dense_model.boundingBox();
    if (!bounds.valid) {
      report.error_message = "CompressionQuality reference bounds are invalid.";
      return report;
    }
    for (int ix = 0; ix < axis; ++ix) {
      for (int iy = 0; iy < axis; ++iy) {
        for (int iz = 0; iz < axis; ++iz) {
          const double tx =
              static_cast<double>(ix) / static_cast<double>(axis - 1);
          const double ty =
              static_cast<double>(iy) / static_cast<double>(axis - 1);
          const double tz =
              static_cast<double>(iz) / static_cast<double>(axis - 1);
          const Vector3 p{
              bounds.min.x + tx * (bounds.max.x - bounds.min.x),
              bounds.min.y + ty * (bounds.max.y - bounds.min.y),
              bounds.min.z + tz * (bounds.max.z - bounds.min.z)};
          addSample(
              reference_dense_model,
              compressed_model,
              p,
              options,
              &report,
              &errors,
              &sum_abs,
              &sum_sq);
        }
      }
    }
  }

  if (report.sample_count == 0) {
    report.error_message = "CompressionQuality produced no finite samples.";
    return report;
  }
  report.mean_abs_error = sum_abs / static_cast<double>(report.sample_count);
  report.rms_error = std::sqrt(sum_sq / static_cast<double>(report.sample_count));
  report.p95_abs_error = percentile(errors, 0.95);
  const std::size_t compressed_memory = compressed_model.memoryFootprintBytes();
  if (compressed_memory > 0) {
    report.compression_ratio =
        static_cast<double>(reference_dense_model.memoryFootprintBytes()) /
        static_cast<double>(compressed_memory);
  }
  report.success = true;
  return report;
}

}  // namespace adasdf

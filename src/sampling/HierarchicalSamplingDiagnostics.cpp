#include "adasdf/sampling/HierarchicalSamplingDiagnostics.h"

#include <algorithm>

namespace adasdf {

const char* toString(FarFieldQualityCheckMode mode) {
  switch (mode) {
    case FarFieldQualityCheckMode::None:
      return "none";
    case FarFieldQualityCheckMode::Corners:
      return "corners";
    case FarFieldQualityCheckMode::Sparse:
      return "sparse";
    case FarFieldQualityCheckMode::Full:
      return "full";
  }
  return "corners";
}

bool parseFarFieldQualityCheckMode(
    const std::string& value,
    FarFieldQualityCheckMode* mode) {
  if (value == "none") {
    *mode = FarFieldQualityCheckMode::None;
    return true;
  }
  if (value == "corners") {
    *mode = FarFieldQualityCheckMode::Corners;
    return true;
  }
  if (value == "sparse") {
    *mode = FarFieldQualityCheckMode::Sparse;
    return true;
  }
  if (value == "full") {
    *mode = FarFieldQualityCheckMode::Full;
    return true;
  }
  return false;
}

void finalizeHierarchicalSamplingDiagnostics(
    HierarchicalSamplingDiagnostics* diagnostics) {
  if (diagnostics == nullptr) {
    return;
  }
  if (diagnostics->exact_reference_time_ms > 0.0 &&
      diagnostics->total_hierarchical_time_ms > 0.0) {
    diagnostics->speedup_vs_exact =
        diagnostics->exact_reference_time_ms /
        diagnostics->total_hierarchical_time_ms;
  }
  const std::size_t predicted_blocks =
      diagnostics->predicted_block_count == 0
          ? diagnostics->accepted_prediction_block_count +
                diagnostics->fallback_exact_block_count
          : diagnostics->predicted_block_count;
  if (predicted_blocks > 0) {
    diagnostics->prediction_acceptance_rate =
        static_cast<double>(diagnostics->accepted_prediction_block_count) /
        static_cast<double>(predicted_blocks);
    diagnostics->fallback_rate =
        static_cast<double>(diagnostics->fallback_exact_block_count) /
        static_cast<double>(predicted_blocks);
  }
  const std::size_t reference_samples =
      diagnostics->fine_sample_count == 0
          ? diagnostics->exact_bvh_sample_count
          : diagnostics->fine_sample_count;
  if (reference_samples > 0) {
    diagnostics->exact_sample_reduction_ratio =
        1.0 -
        static_cast<double>(diagnostics->exact_bvh_sample_count) /
            static_cast<double>(reference_samples);
    diagnostics->exact_sample_reduction_ratio =
        std::max(-1.0, std::min(1.0, diagnostics->exact_sample_reduction_ratio));
  }
}

}  // namespace adasdf

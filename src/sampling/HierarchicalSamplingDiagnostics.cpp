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

const char* toString(NearSurfaceSamplingMode mode) {
  switch (mode) {
    case NearSurfaceSamplingMode::Exact:
      return "exact";
    case NearSurfaceSamplingMode::Banded:
      return "banded";
  }
  return "exact";
}

bool parseNearSurfaceSamplingMode(
    const std::string& value,
    NearSurfaceSamplingMode* mode) {
  if (value == "exact") {
    *mode = NearSurfaceSamplingMode::Exact;
    return true;
  }
  if (value == "banded") {
    *mode = NearSurfaceSamplingMode::Banded;
    return true;
  }
  return false;
}

const char* toString(FarFieldSignPolicy policy) {
  switch (policy) {
    case FarFieldSignPolicy::Exact:
      return "exact";
    case FarFieldSignPolicy::ReuseCoarse:
      return "reuse-coarse";
    case FarFieldSignPolicy::Constant:
      return "constant";
  }
  return "exact";
}

bool parseFarFieldSignPolicy(
    const std::string& value,
    FarFieldSignPolicy* policy) {
  if (value == "exact") {
    *policy = FarFieldSignPolicy::Exact;
    return true;
  }
  if (value == "reuse-coarse") {
    *policy = FarFieldSignPolicy::ReuseCoarse;
    return true;
  }
  if (value == "constant") {
    *policy = FarFieldSignPolicy::Constant;
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
  const std::size_t near_surface_predicted =
      diagnostics->near_surface_predicted_node_count;
  const std::size_t near_surface_total =
      diagnostics->near_surface_local_exact_node_count +
      near_surface_predicted;
  if (near_surface_total > 0) {
    diagnostics->near_surface_local_prediction_acceptance_rate =
        static_cast<double>(near_surface_predicted) /
        static_cast<double>(near_surface_total);
  }
}

}  // namespace adasdf

#pragma once

#include <cstddef>
#include <string>

namespace adasdf {

enum class FarFieldQualityCheckMode {
  None,
  Corners,
  Sparse,
  Full
};

enum class NearSurfaceSamplingMode {
  Exact,
  Banded
};

enum class FarFieldSignPolicy {
  Exact,
  ReuseCoarse,
  Constant
};

const char* toString(FarFieldQualityCheckMode mode);
bool parseFarFieldQualityCheckMode(
    const std::string& value,
    FarFieldQualityCheckMode* mode);
const char* toString(NearSurfaceSamplingMode mode);
bool parseNearSurfaceSamplingMode(
    const std::string& value,
    NearSurfaceSamplingMode* mode);
const char* toString(FarFieldSignPolicy policy);
bool parseFarFieldSignPolicy(
    const std::string& value,
    FarFieldSignPolicy* policy);

struct HierarchicalSamplingDiagnostics {
  std::size_t total_block_count = 0;

  std::size_t near_surface_block_count = 0;
  std::size_t transition_block_count = 0;
  std::size_t far_field_block_count = 0;

  std::size_t exact_block_count = 0;
  std::size_t predicted_block_count = 0;
  std::size_t accepted_prediction_block_count = 0;
  std::size_t fallback_exact_block_count = 0;

  std::size_t coarse_sample_count = 0;
  std::size_t fine_sample_count = 0;
  std::size_t exact_bvh_sample_count = 0;
  std::size_t predicted_sample_count = 0;
  std::size_t quality_check_sample_count = 0;
  std::size_t reused_coarse_sample_count = 0;
  std::size_t skipped_far_field_quality_check_count = 0;

  std::size_t near_surface_banded_block_count = 0;
  std::size_t near_surface_local_exact_node_count = 0;
  std::size_t near_surface_predicted_node_count = 0;
  std::size_t near_surface_local_fallback_node_count = 0;

  std::size_t distance_query_count = 0;
  std::size_t sign_query_count = 0;
  std::size_t triangle_distance_test_count = 0;
  std::size_t bvh_node_visit_count = 0;

  std::size_t fallback_due_to_error_count = 0;
  std::size_t fallback_due_to_sign_count = 0;
  std::size_t fallback_due_to_near_surface_count = 0;
  std::size_t fallback_due_to_invalid_prediction_count = 0;

  double classification_time_ms = 0.0;
  double coarse_sampling_time_ms = 0.0;
  double prediction_time_ms = 0.0;
  double quality_check_time_ms = 0.0;
  double fallback_exact_time_ms = 0.0;
  double exact_sampling_time_ms = 0.0;
  double report_time_ms = 0.0;
  double total_hierarchical_time_ms = 0.0;

  double exact_reference_time_ms = 0.0;
  double speedup_vs_exact = 0.0;

  double exact_sample_reduction_ratio = 0.0;
  double prediction_acceptance_rate = 0.0;
  double fallback_rate = 0.0;
  double near_surface_local_prediction_acceptance_rate = 0.0;
};

void finalizeHierarchicalSamplingDiagnostics(
    HierarchicalSamplingDiagnostics* diagnostics);

}  // namespace adasdf

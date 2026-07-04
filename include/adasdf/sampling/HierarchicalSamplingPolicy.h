#pragma once

#include <string>
#include <vector>

#include "adasdf/sampling/BlockClassification.h"

namespace adasdf {

enum class BlockSamplingMode {
  ExactBVH,
  CoarsePredictThenCheck,
  InterpolatedFarField
};

const char* toString(BlockSamplingMode mode);

struct HierarchicalSamplingOptions {
  bool enable_hierarchical_sampling = false;

  int coarse_resolution = 3;
  int quality_check_samples_per_axis = 3;

  double target_max_abs_error = 1e-3;
  double target_rms_error = 5e-4;
  double target_p95_error = 1e-3;

  double near_surface_band = 1e-3;
  double near_surface_error_factor = 1.0;
  double transition_error_factor = 2.0;
  double far_field_error_factor = 10.0;

  bool allow_far_field_interpolation = true;
  bool allow_transition_prediction = true;

  bool quality_guard = true;
  bool fallback_to_exact_on_quality_fail = true;

  bool keep_near_surface_exact = true;

  int threads = 1;
};

struct BlockSamplingDecision {
  int block_id = -1;
  BlockImportanceClass importance = BlockImportanceClass::FarField;
  BlockSamplingMode mode = BlockSamplingMode::ExactBVH;

  bool quality_guard_required = true;
  double target_error_for_block = 1e-3;

  std::vector<std::string> rationale;
};

class HierarchicalSamplingPolicy {
 public:
  static BlockSamplingDecision decide(
      int block_id,
      const BlockClassificationResult& classification,
      const HierarchicalSamplingOptions& options);
};

}  // namespace adasdf

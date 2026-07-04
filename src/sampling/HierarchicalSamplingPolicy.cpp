#include "adasdf/sampling/HierarchicalSamplingPolicy.h"

#include <algorithm>

namespace adasdf {

const char* toString(BlockSamplingMode mode) {
  switch (mode) {
    case BlockSamplingMode::ExactBVH:
      return "exact-bvh";
    case BlockSamplingMode::CoarsePredictThenCheck:
      return "coarse-predict-then-check";
    case BlockSamplingMode::InterpolatedFarField:
      return "interpolated-far-field";
  }
  return "exact-bvh";
}

BlockSamplingDecision HierarchicalSamplingPolicy::decide(
    int block_id,
    const BlockClassificationResult& classification,
    const HierarchicalSamplingOptions& options) {
  BlockSamplingDecision decision;
  decision.block_id = block_id;
  decision.importance = classification.importance;
  decision.target_error_for_block =
      std::max(0.0, options.target_max_abs_error);

  if (!options.enable_hierarchical_sampling) {
    decision.mode = BlockSamplingMode::ExactBVH;
    decision.rationale.push_back("hierarchical sampling disabled");
    return decision;
  }
  if (!options.quality_guard) {
    decision.mode = BlockSamplingMode::ExactBVH;
    decision.rationale.push_back(
        "quality guard disabled; exact sampling required for safety");
    return decision;
  }

  if (classification.importance == BlockImportanceClass::NearSurface) {
    decision.target_error_for_block =
        options.target_max_abs_error * options.near_surface_error_factor;
    if (options.keep_near_surface_exact &&
        options.near_surface_mode == NearSurfaceSamplingMode::Exact) {
      decision.mode = BlockSamplingMode::ExactBVH;
      decision.rationale.push_back("near-surface block kept exact");
      return decision;
    }
    if (options.near_surface_mode == NearSurfaceSamplingMode::Banded) {
      decision.mode = BlockSamplingMode::CoarsePredictThenCheck;
      decision.rationale.push_back(
          "near-surface block uses experimental banded local exact sampling");
      return decision;
    }
  }

  if (classification.importance == BlockImportanceClass::Transition) {
    decision.target_error_for_block =
        options.target_max_abs_error * options.transition_error_factor;
    if (options.allow_transition_prediction) {
      decision.mode = BlockSamplingMode::CoarsePredictThenCheck;
      decision.rationale.push_back("transition block uses prediction plus guard");
    } else {
      decision.mode = BlockSamplingMode::ExactBVH;
      decision.rationale.push_back("transition prediction disabled");
    }
    return decision;
  }

  decision.target_error_for_block =
      options.target_max_abs_error * options.far_field_error_factor;
  if (options.allow_far_field_interpolation) {
    decision.mode = BlockSamplingMode::InterpolatedFarField;
    decision.rationale.push_back("far-field block uses guarded interpolation");
  } else {
    decision.mode = BlockSamplingMode::ExactBVH;
    decision.rationale.push_back("far-field interpolation disabled");
  }
  return decision;
}

}  // namespace adasdf

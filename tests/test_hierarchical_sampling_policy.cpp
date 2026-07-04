#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::HierarchicalSamplingOptions options;
  options.enable_hierarchical_sampling = true;
  options.target_max_abs_error = 0.01;

  adasdf::BlockClassificationResult near_surface;
  near_surface.importance = adasdf::BlockImportanceClass::NearSurface;
  const auto near_decision =
      adasdf::HierarchicalSamplingPolicy::decide(1, near_surface, options);
  if (near_decision.mode != adasdf::BlockSamplingMode::ExactBVH) {
    std::cerr << "near-surface blocks should remain exact by default\n";
    return 1;
  }

  adasdf::BlockClassificationResult transition;
  transition.importance = adasdf::BlockImportanceClass::Transition;
  const auto transition_decision =
      adasdf::HierarchicalSamplingPolicy::decide(2, transition, options);
  if (transition_decision.mode !=
      adasdf::BlockSamplingMode::CoarsePredictThenCheck) {
    std::cerr << "transition block should use guarded prediction\n";
    return 1;
  }

  options.quality_guard = false;
  const auto unguarded =
      adasdf::HierarchicalSamplingPolicy::decide(3, transition, options);
  if (unguarded.mode != adasdf::BlockSamplingMode::ExactBVH) {
    std::cerr << "prediction without quality guard must fall back to exact\n";
    return 1;
  }

  std::cout << "hierarchical sampling policy passed\n";
  return 0;
}

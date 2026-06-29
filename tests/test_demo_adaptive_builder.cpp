#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  try {
    adasdf::DemoAdaptiveBuildRequest default_request;
    default_request.use_surrogate = true;
    const auto default_build =
        adasdf::DemoAdaptiveSDFBuilder::build(default_request);

    adasdf::DemoAdaptiveBuildRequest small_memory;
    small_memory.use_surrogate = true;
    small_memory.memory_limit_mb = 16.0;
    small_memory.block_expand_limit_mb = 4.0;
    const auto constrained =
        adasdf::DemoAdaptiveSDFBuilder::build(small_memory);

    if (default_build.model->blocks().empty() ||
        constrained.model->blocks().empty()) {
      std::cerr << "demo adaptive builder returned no blocks\n";
      return 1;
    }
    if (constrained.model->blocks().size() >
        default_build.model->blocks().size()) {
      std::cerr << "smaller memory should not increase block count\n";
      return 1;
    }
    if (constrained.candidate.options.max_rank >
        default_build.candidate.options.max_rank) {
      std::cerr << "smaller memory should not increase rank\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_demo_adaptive_builder failed: "
              << exc.what() << "\n";
    return 1;
  }
}

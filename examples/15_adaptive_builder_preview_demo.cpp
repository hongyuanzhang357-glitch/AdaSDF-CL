#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::AdaptiveSDFBuildOptions options;
    options.target_near_surface_error = 1.0e-3;
    options.memory_budget_mb = 512.0;
    options.enable_low_rank_compression = true;

    const adasdf::AdaptiveSDFBuildPlan plan =
        adasdf::AdaptiveSDFBuilderPreview::makePlan(options);
    std::cout << "AdaSDF-CL adaptive builder preview demo\n";
    std::cout << adasdf::AdaptiveSDFBuilderPreview::planToMarkdown(plan);
    std::cout
        << "Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF "
           "construction.\n";
    std::cout
        << "Low-rank compression is planned for v1.7.0-alpha.\n";
    return plan.valid && plan.implemented_in_this_version ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_adaptive_builder_preview_demo failed: "
              << exc.what() << "\n";
    return 1;
  }
}

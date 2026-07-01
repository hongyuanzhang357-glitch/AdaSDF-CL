#include <adasdf/adasdf.h>

#include <algorithm>
#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::AdaptiveSDFBuildOptions options;
    options.enable_low_rank_compression = true;
    const adasdf::AdaptiveSDFBuildPlan plan =
        adasdf::AdaptiveSDFBuilderPreview::makePlan(options);
    const auto has_stage = [&](adasdf::AdaptiveBuilderStage stage) {
      return std::find(plan.stages.begin(), plan.stages.end(), stage) !=
             plan.stages.end();
    };
    const std::string markdown =
        adasdf::AdaptiveSDFBuilderPreview::planToMarkdown(plan);
    if (!plan.valid || plan.implemented_in_this_version ||
        !has_stage(adasdf::AdaptiveBuilderStage::OctreeRefinement) ||
        !has_stage(adasdf::AdaptiveBuilderStage::BlockPartition) ||
        !has_stage(adasdf::AdaptiveBuilderStage::LowRankCompression) ||
        markdown.find("interface preview only") == std::string::npos) {
      std::cerr << "adaptive preview plan failed\n";
      return 1;
    }
    std::cout << "adaptive builder preview passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_builder_preview failed: " << exc.what() << "\n";
    return 1;
  }
}

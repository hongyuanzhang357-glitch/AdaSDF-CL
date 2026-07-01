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
    if (!plan.valid || !plan.implemented_in_this_version ||
        !has_stage(adasdf::AdaptiveBuilderStage::OctreeRefinement) ||
        !has_stage(adasdf::AdaptiveBuilderStage::BlockPartition) ||
        !has_stage(adasdf::AdaptiveBuilderStage::LowRankCompression) ||
        !has_stage(adasdf::AdaptiveBuilderStage::TuckerCompression) ||
        markdown.find("OctreeRefinement implemented in v1.6.0-alpha") ==
            std::string::npos ||
        markdown.find("LowRankCompression implemented in v1.7.0-alpha") ==
            std::string::npos ||
        markdown.find("SurrogateRecommendation implemented in v1.8.0-alpha") ==
            std::string::npos ||
        markdown.find("Tucker/HOSVD compression is not implemented") ==
            std::string::npos) {
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

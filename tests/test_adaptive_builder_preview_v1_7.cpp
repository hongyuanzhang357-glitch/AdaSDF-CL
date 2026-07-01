#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBuildOptions options;
    const adasdf::AdaptiveSDFBuildPlan plan =
        adasdf::AdaptiveSDFBuilderPreview::makePlan(options);
    const std::string markdown =
        adasdf::AdaptiveSDFBuilderPreview::planToMarkdown(plan);
    if (!plan.implemented_in_this_version ||
        !contains(markdown, "LowRankCompression implemented in v1.7.0-alpha") ||
        !contains(markdown, "matrix-SVD") ||
        !contains(markdown, "TuckerCompression planned") ||
        !contains(markdown, "SurrogateRecommendation planned") ||
        !contains(markdown, "GPUCompressedQuery planned")) {
      std::cerr << "preview v1.7 wording missing\n";
      return 1;
    }
    std::cout << "adaptive builder preview v1.7 passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_builder_preview_v1_7 failed: "
              << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  try {
    adasdf::DemoAdaptiveBuildRequest request;
    request.use_surrogate = true;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(request);
    const auto model = build.model;
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "demo adaptive model is not valid/queryable\n";
      return 1;
    }
    if (model->octreeNodes().empty() || model->blocks().empty()) {
      std::cerr << "demo adaptive metadata is missing\n";
      return 1;
    }
    if (!std::isfinite(model->sampleDistance({0.0, 0.0, 0.0})) ||
        !model->sampleGradient({0.75, 0.0, 0.0}).allFinite()) {
      std::cerr << "demo adaptive sampling produced non-finite values\n";
      return 1;
    }
    if (model->metadata().format_name !=
        std::string(adasdf::DemoAdaptiveSDFModel::formatMagic())) {
      std::cerr << "demo adaptive format metadata is missing\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_demo_adaptive_sdf_model failed: "
              << exc.what() << "\n";
    return 1;
  }
}

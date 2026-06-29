#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

double maxPhiError(
    const adasdf::BatchQueryOutput& a,
    const adasdf::BatchQueryOutput& b) {
  double error = 0.0;
  for (std::size_t i = 0; i < a.signed_distances.size(); ++i) {
    error = std::max(
        error,
        std::abs(a.signed_distances[i] - b.signed_distances[i]));
  }
  return error;
}

std::vector<adasdf::Vector3> pointsInBox() {
  return {
      {-0.3, -0.2, -0.1},
      {-0.1, 0.1, 0.2},
      {0.0, 0.0, 0.0},
      {0.2, -0.1, 0.1},
      {0.3, 0.2, -0.2}};
}

}  // namespace

int main() {
  const auto model = adasdf::AnalyticSDFModel::createBox();
  const std::vector<adasdf::Vector3> points = pointsInBox();
  const adasdf::BatchQueryOutput reference = adasdf::queryBatchCPU(*model, points);

  adasdf::QueryEngine direct(model, adasdf::QueryModeConfig::cpuDirect());
  if (!direct.prepare()) {
    std::cerr << "CPU direct prepare failed\n";
    return 1;
  }
  if (maxPhiError(reference, direct.queryBatch(points)) > 1.0e-12) {
    std::cerr << "CPU direct query does not match reference\n";
    return 1;
  }

  adasdf::ExpansionOptions options;
  options.global_resolution = 48;
  adasdf::QueryEngine global(
      model,
      adasdf::QueryModeConfig::cpuGlobalExpanded(),
      options);
  if (!global.prepare() || global.stats().expanded_memory_bytes == 0) {
    std::cerr << "CPU global expanded prepare failed\n";
    return 1;
  }
  if (maxPhiError(reference, global.queryBatch(points)) > 0.04) {
    std::cerr << "CPU global expanded query error is too large\n";
    return 1;
  }

  adasdf::DemoAdaptiveBuildRequest request;
  request.use_surrogate = false;
  const auto demo = adasdf::DemoAdaptiveSDFBuilder::build(request);
  adasdf::ExpansionOptions block_options;
  block_options.expansion = adasdf::QueryExpansionMode::Block;
  block_options.block_resolution = 24;
  adasdf::QueryEngine block(
      demo.model,
      adasdf::QueryModeConfig::cpuBlockExpanded(),
      block_options);
  const std::vector<adasdf::Vector3> block_points = pointsInBox();
  const adasdf::BatchQueryOutput block_reference =
      adasdf::queryBatchCPU(*demo.model, block_points);
  if (!block.prepare() || block.stats().expanded_memory_bytes == 0) {
    std::cerr << "CPU block expanded prepare failed\n";
    return 1;
  }
  if (maxPhiError(block_reference, block.queryBatch(block_points)) > 0.05) {
    std::cerr << "CPU block expanded query error is too large\n";
    return 1;
  }

  std::cout << "CPU QueryEngine modes passed\n";
  return 0;
}

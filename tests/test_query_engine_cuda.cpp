#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double maxPhiError(
    const adasdf::BatchQueryOutput& a,
    const adasdf::BatchQueryOutput& b) {
  double error = 0.0;
  for (std::size_t i = 0; i < a.signed_distances.size(); ++i) {
    if (!std::isfinite(b.signed_distances[i])) {
      return std::numeric_limits<double>::infinity();
    }
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
  adasdf::QueryModeConfig invalid;
  invalid.backend = adasdf::QueryBackend::CUDA;
  invalid.expansion = adasdf::QueryExpansionMode::None;
  try {
    adasdf::validateQueryModeConfig(invalid);
    std::cerr << "CUDA none mode was not rejected\n";
    return 1;
  } catch (const std::runtime_error&) {
  }

  if (!adasdf::CudaQueryBackend::isAvailable()) {
    std::cout << "SKIPPED: CUDA query backend unavailable\n";
    return 0;
  }

  const auto model = adasdf::AnalyticSDFModel::createBox();
  const std::vector<adasdf::Vector3> points = pointsInBox();
  const adasdf::BatchQueryOutput reference = adasdf::queryBatchCPU(*model, points);

  adasdf::ExpansionOptions global_options;
  global_options.global_resolution = 48;
  adasdf::QueryEngine global(
      model,
      adasdf::QueryModeConfig::cudaGlobalExpanded(),
      global_options);
  if (!global.prepare() || global.stats().gpu_resident_memory_bytes == 0) {
    std::cerr << "CUDA global expanded prepare failed\n";
    return 1;
  }
  if (maxPhiError(reference, global.queryBatch(points)) > 0.05) {
    std::cerr << "CUDA global expanded query error is too large\n";
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
      adasdf::QueryModeConfig::cudaBlockExpanded(),
      block_options);
  const adasdf::BatchQueryOutput block_reference =
      adasdf::queryBatchCPU(*demo.model, points);
  if (!block.prepare() || block.stats().gpu_resident_memory_bytes == 0) {
    std::cerr << "CUDA block expanded prepare failed\n";
    return 1;
  }
  if (maxPhiError(block_reference, block.queryBatch(points)) > 0.06) {
    std::cerr << "CUDA block expanded query error is too large\n";
    return 1;
  }

  std::cout << "CUDA QueryEngine modes passed\n";
  return 0;
}

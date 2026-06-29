#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  if (!adasdf::CudaQueryBackend::isAvailable()) {
    std::cout << "SKIPPED: CUDA query backend unavailable\n";
    return 0;
  }

  const auto model = adasdf::AnalyticSDFModel::createBox();
  adasdf::BatchQueryInput input;
  input.points = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.5, 0.5, 0.5}};
  const adasdf::BatchQueryOutput output =
      adasdf::CudaQueryBackend::queryAnalyticBox(*model, input);
  if (output.signed_distances.size() != input.points.size() ||
      output.gradients.size() != input.points.size() ||
      output.normals.size() != input.points.size()) {
    std::cerr << "CUDA query output sizes are invalid\n";
    return 1;
  }
  for (std::size_t i = 0; i < input.points.size(); ++i) {
    if (!std::isfinite(output.signed_distances[i]) ||
        !output.gradients[i].allFinite() ||
        !output.normals[i].allFinite()) {
      std::cerr << "CUDA query output contains non-finite values\n";
      return 1;
    }
  }
  std::cout << "CUDA query backend returned finite analytic box results\n";
  return 0;
}

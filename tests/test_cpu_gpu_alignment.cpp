#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

double vectorError(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

int main() {
  if (!adasdf::CudaQueryBackend::isAvailable()) {
    std::cout << "SKIPPED: CUDA query backend unavailable\n";
    return 0;
  }

  const auto model = adasdf::AnalyticSDFModel::createBox();
  adasdf::PointCloudGeneratorOptions options;
  options.num_points = 1000;
  options.seed = 2026;
  options.distribution = adasdf::BenchmarkPointDistribution::Mixed;
  const auto points = adasdf::generateBenchmarkPoints(options);

  adasdf::BatchQueryInput input;
  input.points = points;
  const adasdf::BatchQueryOutput cpu = adasdf::queryBatchCPU(*model, points);
  const adasdf::BatchQueryOutput gpu =
      adasdf::CudaQueryBackend::queryAnalyticBox(*model, input);

  double max_phi_error = 0.0;
  double max_normal_error = 0.0;
  for (std::size_t i = 0; i < points.size(); ++i) {
    max_phi_error = std::max(
        max_phi_error,
        std::abs(cpu.signed_distances[i] - gpu.signed_distances[i]));
    max_normal_error = std::max(
        max_normal_error,
        vectorError(cpu.normals[i], gpu.normals[i]));
  }
  std::cout << "Max phi error: " << max_phi_error << "\n";
  std::cout << "Max normal error: " << max_normal_error << "\n";
  if (max_phi_error > 1.0e-10 || max_normal_error > 1.0e-10) {
    std::cerr << "CPU/GPU batch query alignment exceeded tolerance\n";
    return 1;
  }
  return 0;
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

bool samePoint(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool allFinite(const std::vector<adasdf::Vector3>& points) {
  for (const adasdf::Vector3& point : points) {
    if (!point.allFinite()) {
      return false;
    }
  }
  return true;
}

}  // namespace

int main() {
  adasdf::PointCloudGeneratorOptions options;
  options.num_points = 1000;
  options.seed = 123;
  options.distribution = adasdf::BenchmarkPointDistribution::Mixed;

  const auto a = adasdf::generateBenchmarkPoints(options);
  const auto b = adasdf::generateBenchmarkPoints(options);
  if (a.size() != options.num_points || b.size() != options.num_points ||
      !allFinite(a) || !allFinite(b)) {
    std::cerr << "point generator produced invalid output\n";
    return 1;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!samePoint(a[i], b[i])) {
      std::cerr << "point generator is not deterministic\n";
      return 1;
    }
  }

  options.distribution = adasdf::BenchmarkPointDistribution::NearSurfaceShell;
  const auto shell = adasdf::generateBenchmarkPoints(options);
  options.distribution = adasdf::BenchmarkPointDistribution::UniformBoxVolume;
  const auto volume = adasdf::generateBenchmarkPoints(options);
  if (shell.size() != options.num_points || volume.size() != options.num_points ||
      !allFinite(shell) || !allFinite(volume)) {
    std::cerr << "distribution-specific point generation failed\n";
    return 1;
  }

  std::cout << "point cloud generator is deterministic and finite\n";
  return 0;
}

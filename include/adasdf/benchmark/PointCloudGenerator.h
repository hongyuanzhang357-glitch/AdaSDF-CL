#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

enum class BenchmarkPointDistribution {
  UniformBoxVolume,
  NearSurfaceShell,
  Mixed
};

struct PointCloudGeneratorOptions {
  std::size_t num_points = 10000;
  BenchmarkPointDistribution distribution = BenchmarkPointDistribution::Mixed;
  std::uint32_t seed = 1337;
  Vector3 center = Vector3::Zero();
  Vector3 half_extent{0.5, 0.5, 0.5};
  double volume_scale = 1.8;
  double shell_thickness = 0.05;
};

std::vector<Vector3> generateBenchmarkPoints(
    const PointCloudGeneratorOptions& options);

const char* benchmarkPointDistributionName(
    BenchmarkPointDistribution distribution);

}  // namespace adasdf

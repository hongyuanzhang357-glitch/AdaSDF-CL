#include "adasdf/benchmark/PointCloudGenerator.h"

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

namespace adasdf {
namespace {

double positive(double value, double fallback) {
  return value > 0.0 ? value : fallback;
}

Vector3 uniformVolumePoint(
    std::mt19937& rng,
    const PointCloudGeneratorOptions& options) {
  const double scale = positive(options.volume_scale, 1.0);
  std::uniform_real_distribution<double> unit(-1.0, 1.0);
  return {
      options.center.x + unit(rng) * options.half_extent.x * scale,
      options.center.y + unit(rng) * options.half_extent.y * scale,
      options.center.z + unit(rng) * options.half_extent.z * scale};
}

Vector3 nearSurfacePoint(
    std::mt19937& rng,
    const PointCloudGeneratorOptions& options) {
  std::uniform_int_distribution<int> axis_dist(0, 2);
  std::uniform_int_distribution<int> sign_dist(0, 1);
  std::uniform_real_distribution<double> face_u(-1.0, 1.0);
  std::uniform_real_distribution<double> shell(
      -positive(options.shell_thickness, 1.0e-4),
      positive(options.shell_thickness, 1.0e-4));

  std::array<double, 3> coords = {
      face_u(rng) * options.half_extent.x,
      face_u(rng) * options.half_extent.y,
      face_u(rng) * options.half_extent.z};
  const int axis = axis_dist(rng);
  const double sign = sign_dist(rng) == 0 ? -1.0 : 1.0;
  const std::array<double, 3> extents = {
      options.half_extent.x,
      options.half_extent.y,
      options.half_extent.z};
  coords[axis] = sign * (extents[axis] + shell(rng));
  return {
      options.center.x + coords[0],
      options.center.y + coords[1],
      options.center.z + coords[2]};
}

void validateOptions(const PointCloudGeneratorOptions& options) {
  if (!options.center.allFinite() || !options.half_extent.allFinite()) {
    throw std::runtime_error("benchmark point generator parameters must be finite.");
  }
  if (!(options.half_extent.x > 0.0) ||
      !(options.half_extent.y > 0.0) ||
      !(options.half_extent.z > 0.0)) {
    throw std::runtime_error("benchmark box half extents must be positive.");
  }
}

}  // namespace

std::vector<Vector3> generateBenchmarkPoints(
    const PointCloudGeneratorOptions& options) {
  validateOptions(options);
  std::mt19937 rng(options.seed);
  std::vector<Vector3> points;
  points.reserve(options.num_points);

  for (std::size_t i = 0; i < options.num_points; ++i) {
    switch (options.distribution) {
      case BenchmarkPointDistribution::UniformBoxVolume:
        points.push_back(uniformVolumePoint(rng, options));
        break;
      case BenchmarkPointDistribution::NearSurfaceShell:
        points.push_back(nearSurfacePoint(rng, options));
        break;
      case BenchmarkPointDistribution::Mixed:
        points.push_back(i % 2 == 0 ? nearSurfacePoint(rng, options)
                                    : uniformVolumePoint(rng, options));
        break;
    }
  }
  return points;
}

const char* benchmarkPointDistributionName(
    BenchmarkPointDistribution distribution) {
  switch (distribution) {
    case BenchmarkPointDistribution::UniformBoxVolume:
      return "uniform_box_volume";
    case BenchmarkPointDistribution::NearSurfaceShell:
      return "near_surface_shell";
    case BenchmarkPointDistribution::Mixed:
      return "mixed";
  }
  return "unknown";
}

}  // namespace adasdf

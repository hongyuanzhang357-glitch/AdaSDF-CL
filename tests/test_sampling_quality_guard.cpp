#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

std::size_t idx(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::Vector3 gridPoint(
    const adasdf::AABB& bounds,
    int i,
    int j,
    int k,
    int n) {
  const double tx = static_cast<double>(i) / static_cast<double>(n - 1);
  const double ty = static_cast<double>(j) / static_cast<double>(n - 1);
  const double tz = static_cast<double>(k) / static_cast<double>(n - 1);
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * tx,
      bounds.min.y + (bounds.max.y - bounds.min.y) * ty,
      bounds.min.z + (bounds.max.z - bounds.min.z) * tz};
}

}  // namespace

int main() {
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  if (!read.success) {
    std::cerr << "failed to read cube fixture\n";
    return 1;
  }

  adasdf::BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  adasdf::BVHSDFSampler sampler;
  adasdf::BuildAccelerationStats stats;
  if (!sampler.reset(read.mesh, sampler_options, &stats)) {
    std::cerr << "failed to reset BVH sampler\n";
    return 1;
  }

  const adasdf::AABB bounds{{-0.25, -0.25, -0.25}, {1.25, 1.25, 1.25}, true};
  constexpr int n = 3;
  std::vector<double> exact(n * n * n, 0.0);
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        exact[idx(i, j, k, n, n)] = sampler.sample(gridPoint(bounds, i, j, k, n)).phi;
      }
    }
  }

  adasdf::SamplingQualityOptions quality;
  quality.check_samples_per_axis = n;
  quality.max_abs_error_limit = 1e-12;
  quality.rms_error_limit = 1e-12;
  quality.p95_error_limit = 1e-12;
  const auto report =
      adasdf::SamplingQualityGuard::check(bounds, n, n, n, exact, sampler, quality);
  if (!report.accepted || report.check_sample_count != 27) {
    std::cerr << "exact prediction should pass quality guard\n";
    return 1;
  }

  std::cout << "sampling quality guard passed\n";
  return 0;
}

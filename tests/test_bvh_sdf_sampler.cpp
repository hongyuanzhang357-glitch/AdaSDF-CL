#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

bool near(double a, double b, double eps = 1.0e-9) {
  return std::abs(a - b) <= eps;
}

}  // namespace

int main() {
  try {
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    adasdf::BVHSDFSamplerOptions options;
    options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.signed_distance = true;
    adasdf::BuildAccelerationStats stats;
    adasdf::BVHSDFSampler sampler;
    if (!sampler.reset(read.mesh, options, &stats) || !sampler.hasBVH()) {
      std::cerr << "sampler BVH build failed\n";
      return 1;
    }
    const auto center = sampler.sample({0.5, 0.5, 0.5});
    const auto outside = sampler.sample({1.5, 0.5, 0.5});
    const auto brute =
        adasdf::BVHSDFSampler::sampleBruteForce(read.mesh, {0.5, 0.5, 0.5}, true);
    if (!center.success || !outside.success || !(center.phi < 0.0) ||
        !(outside.phi > 0.0) || !near(center.phi, brute.phi, 1.0e-9)) {
      std::cerr << "BVH SDF sampler failed\n";
      return 1;
    }
    std::cout << "BVH SDF sampler passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_bvh_sdf_sampler failed: " << exc.what() << "\n";
    return 1;
  }
}

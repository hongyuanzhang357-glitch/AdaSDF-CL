#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    adasdf::BVHSDFSamplerOptions options;
    options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.signed_distance = true;
    adasdf::BVHSDFSampler sampler;
    sampler.reset(read.mesh, options);
    const std::vector<adasdf::Vector3> points = {
        {0.25, 0.25, 0.25},
        {0.5, 0.5, 0.5},
        {1.25, 0.5, 0.5},
        {-0.25, 0.25, 0.25},
        {0.25, 1.25, 0.25}};
    for (const adasdf::Vector3& point : points) {
      const auto bvh = sampler.sample(point);
      const auto brute =
          adasdf::BVHSDFSampler::sampleBruteForce(read.mesh, point, true);
      if (!bvh.success || !brute.success ||
          std::abs(bvh.phi - brute.phi) > 1.0e-9) {
        std::cerr << "BVH/bruteforce mismatch\n";
        return 1;
      }
    }
    std::cout << "BVH vs bruteforce consistency passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_bvh_vs_bruteforce_consistency failed: "
              << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <algorithm>
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
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    adasdf::DenseSDFBuildOptions dense_options;
    dense_options.resolution = 22;
    dense_options.padding = 0.05;
    adasdf::DenseSDFBuildReport dense_report;
    auto dense = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        dense_options,
        &dense_report);

    adasdf::AdaptiveBlockSDFBuildOptions adaptive_options;
    adaptive_options.max_octree_level = 2;
    adaptive_options.block_resolution = 6;
    adasdf::AdaptiveBlockSDFBuildReport adaptive_report;
    auto adaptive = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        adaptive_options,
        &adaptive_report);

    if (!dense || !adaptive) {
      std::cerr << "dense/adaptive consistency build failed\n";
      return 1;
    }
    const std::vector<adasdf::Vector3> points = {
        {0.5, 0.5, 0.5},
        {0.25, 0.5, 0.5},
        {1.2, 0.5, 0.5},
        {0.5, 1.2, 0.5},
        {0.5, 0.5, 1.2}};
    double max_diff = 0.0;
    for (const adasdf::Vector3& point : points) {
      max_diff = std::max(
          max_diff,
          std::abs(dense->sampleDistance(point) - adaptive->sampleDistance(point)));
    }
    std::cout << "dense memory bytes: " << dense->memoryFootprintBytes() << "\n";
    std::cout << "adaptive memory bytes: "
              << adaptive->memoryFootprintBytes() << "\n";
    std::cout << "max diff: " << max_diff << "\n";
    if (!(max_diff < 0.30)) {
      std::cerr << "adaptive and dense phi differ too much\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_vs_dense_consistency failed: "
              << exc.what() << "\n";
    return 1;
  }
}

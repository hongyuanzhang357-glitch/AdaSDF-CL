#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 2;
    options.block_resolution = 4;
    options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.threads = 2;
    options.benchmark_brute_reference = true;
    adasdf::AdaptiveBlockSDFBuildReport report;
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    auto model =
        adasdf::AdaptiveBlockSDFBuilder::fromSTL(fixture.string(), options, &report);
    if (!model || !report.success || !report.used_bvh ||
        report.acceleration_stats.bvh_node_count == 0 ||
        report.acceleration_stats.sample_count == 0 ||
        report.acceleration_stats.brute_reference_time_ms <= 0.0) {
      std::cerr << "AdaptiveBlockSDF BVH build failed\n";
      return 1;
    }
    if (!(model->sampleDistance({0.5, 0.5, 0.5}) < 0.0)) {
      std::cerr << "AdaptiveBlockSDF BVH center sign failed\n";
      return 1;
    }
    std::cout << "AdaptiveBlockSDF BVH builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_block_sdf_builder_bvh failed: " << exc.what()
              << "\n";
    return 1;
  }
}

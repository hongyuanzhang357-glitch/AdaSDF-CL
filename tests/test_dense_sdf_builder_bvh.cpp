#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    adasdf::DenseSDFBuildOptions options;
    options.resolution = 8;
    options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.threads = 2;
    adasdf::DenseSDFBuildReport report;
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    auto model =
        adasdf::DenseSDFBuilder::fromSTL(fixture.string(), options, &report);
    if (!model || !report.success || !report.used_bvh ||
        report.acceleration_stats.bvh_node_count == 0 ||
        report.acceleration_stats.sample_count == 0) {
      std::cerr << "DenseSDF BVH build failed\n";
      return 1;
    }
    if (!(model->sampleDistance({0.5, 0.5, 0.5}) < 0.0)) {
      std::cerr << "DenseSDF BVH center sign failed\n";
      return 1;
    }
    std::cout << "DenseSDF BVH builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_dense_sdf_builder_bvh failed: " << exc.what() << "\n";
    return 1;
  }
}

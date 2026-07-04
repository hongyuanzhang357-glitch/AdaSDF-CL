#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 1;
    options.block_resolution = 4;
    options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.hierarchical_sampling.enable_hierarchical_sampling = true;
    options.hierarchical_sampling.coarse_resolution = 2;
    options.hierarchical_sampling.quality_check_samples_per_axis = 2;
    options.hierarchical_sampling.target_max_abs_error = 10.0;
    options.hierarchical_sampling.target_rms_error = 10.0;
    options.hierarchical_sampling.target_p95_error = 10.0;

    adasdf::AdaptiveBlockSDFBuildReport report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        fixture.string(),
        options,
        &report);
    if (!model || !report.success || report.block_count == 0) {
      std::cerr << "hierarchical adaptive build failed: "
                << report.error_message << "\n";
      return 1;
    }
    if (report.hierarchical_sampling.block_count != report.block_count) {
      std::cerr << "hierarchical stats should cover all blocks\n";
      return 1;
    }
    if (report.hierarchical_sampling.exact_sample_count == 0) {
      std::cerr << "hierarchical sampling should perform exact guard samples\n";
      return 1;
    }

    std::cout << "adaptive builder hierarchical sampling passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_builder_hierarchical_sampling failed: "
              << exc.what() << "\n";
    return 1;
  }
}

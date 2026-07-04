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
    adasdf::SpeedQualityBenchmarkOptions options;
    options.build_options.max_octree_level = 1;
    options.build_options.block_resolution = 4;
    options.build_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    options.build_options.hierarchical_sampling.enable_hierarchical_sampling =
        true;
    options.build_options.hierarchical_sampling.allow_far_field_interpolation =
        false;
    options.build_options.hierarchical_sampling.allow_transition_prediction =
        false;
    options.comparison_samples_per_axis = 3;

    const auto result =
        adasdf::SpeedQualityBenchmark::runSTL(fixture.string(), options);
    if (!result.success) {
      std::cerr << "speed/quality benchmark failed: "
                << result.error_message << "\n";
      return 1;
    }
    if (!result.quality_gate_passed || result.max_abs_error > 1e-12 ||
        result.sign_mismatch_count != 0) {
      std::cerr << "exact-fallback hierarchical path should match exact\n";
      return 1;
    }

    std::cout << "hierarchical vs exact quality passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_hierarchical_vs_exact_quality failed: " << exc.what()
              << "\n";
    return 1;
  }
}

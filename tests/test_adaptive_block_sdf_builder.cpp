#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 2;
    options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    if (!model || !report.success || report.block_count == 0 ||
        report.memory_bytes == 0) {
      std::cerr << "closed cube signed adaptive build failed: "
                << report.error_message << "\n";
      return 1;
    }
    if (!(model->sampleDistance({0.5, 0.5, 0.5}) < 0.0)) {
      std::cerr << "center should be inside\n";
      return 1;
    }
    if (!(model->sampleDistance({1.5, 0.5, 0.5}) > 0.0)) {
      std::cerr << "outside point should be positive\n";
      return 1;
    }

    auto signed_open = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "open_cube_missing_face_ascii.stl").string(),
        options,
        &report);
    if (signed_open || report.success) {
      std::cerr << "signed open mesh should fail by default\n";
      return 1;
    }

    options.signed_distance = false;
    auto unsigned_open = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "open_cube_missing_face_ascii.stl").string(),
        options,
        &report);
    if (!unsigned_open || !report.success ||
        !(unsigned_open->sampleDistance({0.5, 0.5, 0.5}) >= 0.0)) {
      std::cerr << "unsigned open mesh should build with non-negative phi\n";
      return 1;
    }
    std::cout << "adaptive block sdf builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_block_sdf_builder failed: " << exc.what()
              << "\n";
    return 1;
  }
}

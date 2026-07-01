#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    adasdf::DenseSDFBuildOptions options;
    options.resolution = 24;
    options.padding = 0.05;
    adasdf::DenseSDFBuildReport report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    if (!model || !report.success || !model->queryBackendAvailable()) {
      std::cerr << "closed cube signed DenseSDF build failed: "
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
    if (!(std::abs(model->sampleDistance({1.0, 0.5, 0.5})) < 0.08)) {
      std::cerr << "surface point should be near zero\n";
      return 1;
    }

    auto signed_open = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "open_cube_missing_face_ascii.stl").string(),
        options,
        &report);
    if (signed_open || report.success) {
      std::cerr << "signed open mesh should fail by default\n";
      return 1;
    }

    options.signed_distance = false;
    auto unsigned_open = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "open_cube_missing_face_ascii.stl").string(),
        options,
        &report);
    if (!unsigned_open || !report.success) {
      std::cerr << "unsigned open mesh should build\n";
      return 1;
    }
    if (!(unsigned_open->sampleDistance({0.5, 0.5, 0.5}) >= 0.0)) {
      std::cerr << "unsigned distance should be non-negative\n";
      return 1;
    }
    std::cout << "dense sdf builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_dense_sdf_builder failed: " << exc.what() << "\n";
    return 1;
  }
}

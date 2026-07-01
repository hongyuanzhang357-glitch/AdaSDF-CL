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
    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 2;
    options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport report;
    auto base = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    auto model = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(base);
    if (!model || !model->isValid()) {
      std::cerr << "adaptive model build failed\n";
      return 1;
    }
    if (!std::isfinite(model->sampleDistance({0.5, 0.5, 0.5})) ||
        !model->sampleGradient({0.5, 0.5, 0.5}).allFinite()) {
      std::cerr << "adaptive model query produced non-finite values\n";
      return 1;
    }
    if (model->findContainingBlock({0.5, 0.5, 0.5}) < 0) {
      std::cerr << "inside-domain point did not map to a block\n";
      return 1;
    }
    if (!std::isfinite(model->sampleDistance({2.0, 2.0, 2.0}))) {
      std::cerr << "outside-domain fallback produced non-finite value\n";
      return 1;
    }
    if (!model->boundingBox().valid) {
      std::cerr << "adaptive model AABB invalid\n";
      return 1;
    }
    std::cout << "adaptive block sdf model passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_block_sdf_model failed: " << exc.what()
              << "\n";
    return 1;
  }
}

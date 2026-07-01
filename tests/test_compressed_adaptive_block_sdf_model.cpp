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
    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    build_options.max_octree_level = 2;
    build_options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        build_options,
        &build_report);
    auto adaptive = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
    if (!adaptive) {
      std::cerr << "adaptive build failed\n";
      return 1;
    }
    adasdf::BlockLowRankCompressionOptions options;
    options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &report);
    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    if (!compressed_model.isValid() || !compressed_model.queryBackendAvailable()) {
      std::cerr << "compressed model invalid\n";
      return 1;
    }
    const adasdf::Vector3 center{0.5, 0.5, 0.5};
    const double ref = adaptive->sampleDistance(center);
    const double cmp = compressed_model.sampleDistance(center);
    if (!std::isfinite(cmp) || std::abs(ref - cmp) > 1.0e-2) {
      std::cerr << "compressed query differs too much\n";
      return 1;
    }
    if (!compressed_model.sampleGradient(center).allFinite() ||
        compressed_model.findContainingBlock(center) < 0 ||
        !std::isfinite(compressed_model.sampleDistance({2.0, 2.0, 2.0}))) {
      std::cerr << "compressed model query helpers failed\n";
      return 1;
    }
    std::cout << "compressed adaptive block sdf model passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compressed_adaptive_block_sdf_model failed: "
              << exc.what() << "\n";
    return 1;
  }
}

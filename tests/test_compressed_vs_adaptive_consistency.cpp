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
    adasdf::BlockLowRankCompressionOptions options;
    options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &report);
    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    const std::vector<adasdf::Vector3> points = {
        {0.5, 0.5, 0.5},
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 1.0},
        {1.2, 0.5, 0.5},
        {0.25, 0.25, 0.25}};
    double max_diff = 0.0;
    std::size_t sign_mismatch = 0;
    for (const adasdf::Vector3& p : points) {
      const double a = adaptive->sampleDistance(p);
      const double c = compressed_model.sampleDistance(p);
      if (!std::isfinite(a) || !std::isfinite(c)) {
        std::cerr << "non-finite consistency sample\n";
        return 1;
      }
      max_diff = std::max(max_diff, std::abs(a - c));
      if (std::abs(a) > 1.0e-10 && std::abs(c) > 1.0e-10 &&
          ((a < 0.0) != (c < 0.0))) {
        ++sign_mismatch;
      }
    }
    if (max_diff > 5.0e-2 || sign_mismatch > 0 ||
        !std::isfinite(compressed_model.compressedBlockSet().compressionRatio())) {
      std::cerr << "compressed vs adaptive mismatch too large\n";
      return 1;
    }
    std::cout << "compressed vs adaptive consistency passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compressed_vs_adaptive_consistency failed: "
              << exc.what() << "\n";
    return 1;
  }
}

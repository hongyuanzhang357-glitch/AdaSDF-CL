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
    adasdf::BlockLowRankCompressionOptions options;
    options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &compression_report);
    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    adasdf::CompressionQualityOptions quality_options;
    quality_options.samples_per_block_axis = 3;
    adasdf::CompressionQualityReport quality =
        adasdf::CompressionQuality::compare(*adaptive, compressed_model, quality_options);
    if (!quality.success || quality.sample_count == 0 ||
        !std::isfinite(quality.max_abs_error) ||
        !std::isfinite(quality.compression_ratio)) {
      std::cerr << "compression quality report invalid\n";
      return 1;
    }
    std::cout << "compression quality passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compression_quality failed: " << exc.what() << "\n";
    return 1;
  }
}

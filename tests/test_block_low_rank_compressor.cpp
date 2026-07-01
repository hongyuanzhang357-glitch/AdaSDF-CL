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
    options.target_max_abs_error = 1.0e-8;
    options.target_rms_error = 1.0e-10;
    adasdf::BlockLowRankCompressionReport report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &report);
    if (!report.success ||
        report.compressed_block_count + report.dense_fallback_block_count !=
            report.input_block_count ||
        !std::isfinite(report.compression_ratio) ||
        compressed.blockCount() != adaptive->blockSet().blockCount()) {
      std::cerr << "compression report invalid\n";
      return 1;
    }
    if (report.compressed_block_count == 0 || report.ranks_used.empty()) {
      std::cerr << "expected at least one MatrixSVD block\n";
      return 1;
    }

    options.max_rank = 1;
    options.target_max_abs_error = 1.0e-16;
    options.target_rms_error = 1.0e-16;
    compressed = adasdf::BlockLowRankCompressor::compress(
        adaptive->blockSet(),
        options,
        &report);
    if (!report.success || report.dense_fallback_block_count == 0) {
      std::cerr << "strict compression should allow dense fallback\n";
      return 1;
    }
    std::cout << "block low rank compressor passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_block_low_rank_compressor failed: " << exc.what() << "\n";
    return 1;
  }
}

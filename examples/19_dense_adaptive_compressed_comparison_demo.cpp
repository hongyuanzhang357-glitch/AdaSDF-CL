#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  try {
    const std::filesystem::path source_dir = ADASDF_CL_SOURCE_DIR;
    const std::filesystem::path fixture =
        source_dir / "tests" / "data" / "mesh_diagnostics" /
        "closed_cube_ascii.stl";

    adasdf::DenseSDFBuildOptions dense_options;
    dense_options.resolution = 16;
    adasdf::DenseSDFBuildReport dense_report;
    auto dense = adasdf::DenseSDFBuilder::fromSTL(
        fixture.string(),
        dense_options,
        &dense_report);

    adasdf::AdaptiveBlockSDFBuildOptions adaptive_options;
    adaptive_options.max_octree_level = 2;
    adaptive_options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport adaptive_report;
    auto adaptive_model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        fixture.string(),
        adaptive_options,
        &adaptive_report);
    auto adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(adaptive_model);

    adasdf::BlockLowRankCompressionOptions compression_options;
    compression_options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            compression_options,
            &compression_report);
    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));

    const std::vector<adasdf::Vector3> points = {
        {0.5, 0.5, 0.5},
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 1.0},
        {1.2, 0.5, 0.5}};
    double max_adaptive_compressed_diff = 0.0;
    for (const adasdf::Vector3& p : points) {
      max_adaptive_compressed_diff = std::max(
          max_adaptive_compressed_diff,
          std::abs(adaptive->sampleDistance(p) -
                   compressed_model.sampleDistance(p)));
    }

    std::cout << "AdaSDF-CL dense/adaptive/compressed comparison demo\n";
    std::cout << "Dense memory bytes: " << dense->memoryFootprintBytes() << "\n";
    std::cout << "Adaptive dense memory bytes: "
              << adaptive->memoryFootprintBytes() << "\n";
    std::cout << "Compressed memory bytes: "
              << compressed_model.memoryFootprintBytes() << "\n";
    std::cout << "Compression ratio: "
              << compression_report.compression_ratio << "\n";
    std::cout << "Max adaptive/compressed phi diff: "
              << max_adaptive_compressed_diff << "\n";
    return dense && adaptive && compressed_model.isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "dense/adaptive/compressed comparison demo failed: "
              << exc.what() << "\n";
    return 1;
  }
}

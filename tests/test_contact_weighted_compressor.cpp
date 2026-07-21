#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::AdaptiveSDFBlock block;
    block.block_id = 1;
    block.level = 0;
    block.bounds.valid = true;
    block.bounds.min = {0.0, 0.0, 0.0};
    block.bounds.max = {1.0, 1.0, 1.0};
    block.origin = block.bounds.min;
    block.spacing = {1.0, 1.0, 1.0};
    block.nx = 2;
    block.ny = 2;
    block.nz = 2;
    block.near_surface = true;
    block.phi.assign(8, 0.25);
    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.global_bounds = block.bounds;
    blocks.blocks.push_back(block);

    adasdf::NarrowBandBrickBuildOptions options;
    options.compression_mode = adasdf::NarrowBandCompressionMode::ExistingLowRank;
    options.max_rank = 8;
    adasdf::ContactWeightedCompressionReport report;
    adasdf::AdaptiveSDFBlockSet out =
        adasdf::ContactWeightedCompressor::compressCompatible(
            std::move(blocks),
            options,
            &report);
    if (out.blocks.size() != 1 ||
        report.dense_fallback_block_count != 1 ||
        report.compressed_block_count != 0 ||
        report.contact_band_sign_flip_count != 0 ||
        report.estimated_expanded_bytes == 0) {
      std::cerr << "contact weighted compressor prototype stats failed\n";
      return 1;
    }
    std::cout << "contact weighted compressor passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_weighted_compressor failed: "
              << exc.what() << "\n";
    return 1;
  }
}

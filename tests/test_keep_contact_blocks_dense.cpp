#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(int block_id, double x0) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = block_id;
  block.octree_node_id = block_id;
  block.level = 1;
  block.bounds = {{x0, 0.0, 0.0}, {x0 + 1.0, 1.0, 1.0}, true};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.origin = {x0, 0.0, 0.0};
  block.spacing = {0.5, 0.5, 0.5};
  block.near_surface = true;
  block.signed_distance = true;
  block.phi.reserve(27);
  for (int k = 0; k < block.nz; ++k) {
    for (int j = 0; j < block.ny; ++j) {
      for (int i = 0; i < block.nx; ++i) {
        block.phi.push_back(
            0.01 * static_cast<double>(i) -
            0.02 * static_cast<double>(j) +
            0.03 * static_cast<double>(k) +
            0.001 * static_cast<double>(block_id));
      }
    }
  }
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.global_bounds = {{0.0, 0.0, 0.0}, {2.0, 1.0, 1.0}, true};
    blocks.signed_distance = true;
    blocks.blocks.push_back(makeBlock(11, 0.0));
    blocks.blocks.push_back(makeBlock(12, 1.0));

    adasdf::BlockLowRankCompressionOptions options;
    options.min_rank = 1;
    options.max_rank = 2;
    options.target_max_abs_error = 1.0;
    options.target_rms_error = 1.0;
    options.target_p95_error = 1.0;
    options.force_dense_block_ids = {11};

    adasdf::BlockLowRankCompressionReport report;
    const adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(blocks, options, &report);

    if (!report.success || compressed.blocks.size() != 2 ||
        report.dense_fallback_block_count != 1 ||
        compressed.blocks[0].method !=
            adasdf::BlockCompressionMethod::DenseFallback ||
        compressed.blocks[1].method !=
            adasdf::BlockCompressionMethod::MatrixSVD) {
      std::cerr << "forced contact block dense fallback failed\n";
      return 1;
    }

    std::cout << "keep contact blocks dense passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_keep_contact_blocks_dense failed: " << exc.what()
              << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

namespace {

adasdf::AdaptiveSDFBlock makeBlock() {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 7;
  block.octree_node_id = 7;
  block.level = 1;
  block.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  block.nx = 4;
  block.ny = 4;
  block.nz = 4;
  block.origin = {0.0, 0.0, 0.0};
  block.spacing = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
  block.near_surface = true;
  block.signed_distance = true;
  block.phi.reserve(64);
  for (int k = 0; k < block.nz; ++k) {
    for (int j = 0; j < block.ny; ++j) {
      for (int i = 0; i < block.nx; ++i) {
        const double value =
            0.02 * std::sin(static_cast<double>(i + 2 * j + 3 * k + 1)) +
            0.003 * static_cast<double>((i * j + k) % 3 - 1);
        block.phi.push_back(value);
      }
    }
  }
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBlockSet blocks;
    blocks.global_bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
    blocks.signed_distance = true;
    blocks.blocks.push_back(makeBlock());

    adasdf::BlockLowRankCompressionOptions options;
    options.min_rank = 1;
    options.max_rank = 1;
    options.target_max_abs_error = 1.0;
    options.target_rms_error = 1.0;
    options.target_p95_error = 1.0;
    options.near_zero_compression_guard = true;
    options.compression_sign_guard_band = 1.0;
    options.compression_near_zero_error_limit = 0.0;
    options.compression_sign_guard_action =
        adasdf::CompressionSignGuardAction::KeepDense;

    adasdf::BlockLowRankCompressionReport report;
    const adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(blocks, options, &report);

    if (!report.success || !report.compression_guard_enabled ||
        report.guarded_block_count != 1 ||
        report.dense_fallback_block_count != 1 ||
        report.kept_dense_due_to_error_count != 1 ||
        compressed.blocks.size() != 1 ||
        compressed.blocks[0].method !=
            adasdf::BlockCompressionMethod::DenseFallback ||
        report.dense_fallback_memory_bytes == 0 ||
        report.compressed_memory_bytes_after_guard == 0) {
      std::cerr << "near-zero compression guard did not keep block dense\n";
      return 1;
    }

    std::cout << "compression near-zero guard passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compression_near_zero_guard failed: " << exc.what()
              << "\n";
    return 1;
  }
}

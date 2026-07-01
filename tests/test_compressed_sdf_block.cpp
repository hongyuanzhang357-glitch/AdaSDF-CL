#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  std::vector<double> matrix = {
      1.0, 2.0, 3.0, 4.0,
      2.0, 4.0, 6.0, 8.0};
  adasdf::SmallSVDOptions options;
  options.max_rank = 1;
  adasdf::SmallSVDResult svd =
      adasdf::SmallMatrixSVD::compute(matrix, 2, 4, options);
  if (!svd.success) {
    std::cerr << svd.error_message << "\n";
    return 1;
  }

  adasdf::CompressedSDFBlock block;
  block.block_id = 0;
  block.source_block_id = 0;
  block.nx = 2;
  block.ny = 2;
  block.nz = 2;
  block.method = adasdf::BlockCompressionMethod::MatrixSVD;
  block.svd.rows = 2;
  block.svd.cols = 4;
  block.svd.rank = svd.rank;
  block.svd.nx = 2;
  block.svd.ny = 2;
  block.svd.nz = 2;
  block.svd.U = svd.U;
  block.svd.S = svd.S;
  block.svd.Vt = svd.Vt;
  if (std::abs(adasdf::compressedBlockGridValue(block, 1, 1, 0) - 6.0) >
      1.0e-8) {
    std::cerr << "MatrixSVD block reconstruction failed\n";
    return 1;
  }

  adasdf::CompressedSDFBlock dense = block;
  dense.method = adasdf::BlockCompressionMethod::DenseFallback;
  dense.dense_phi = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  if (adasdf::compressedBlockGridValue(dense, 1, 1, 1) != 8.0) {
    std::cerr << "DenseFallback block reconstruction failed\n";
    return 1;
  }

  adasdf::CompressedAdaptiveBlockSDF set;
  set.blocks.push_back(block);
  set.blocks.push_back(dense);
  if (set.blockCount() != 2 || set.compressedMemoryBytes() == 0 ||
      !std::isfinite(set.compressionRatio())) {
    std::cerr << "compressed block memory stats failed\n";
    return 1;
  }
  std::cout << "compressed sdf block passed\n";
  return 0;
}

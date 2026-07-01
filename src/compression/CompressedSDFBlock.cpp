#include "adasdf/compression/CompressedSDFBlock.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace adasdf {
namespace {

std::size_t denseIndex(int ix, int iy, int iz, int nx, int ny) {
  return static_cast<std::size_t>(ix) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(iy) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(iz));
}

std::size_t matrixIndex(int row, int col, int cols) {
  return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) +
         static_cast<std::size_t>(col);
}

std::size_t denseValueCount(const CompressedSDFBlock& block) {
  return static_cast<std::size_t>(std::max(0, block.nx)) *
         static_cast<std::size_t>(std::max(0, block.ny)) *
         static_cast<std::size_t>(std::max(0, block.nz));
}

}  // namespace

const char* blockCompressionMethodName(BlockCompressionMethod method) {
  switch (method) {
    case BlockCompressionMethod::DenseFallback:
      return "DenseFallback";
    case BlockCompressionMethod::MatrixSVD:
      return "MatrixSVD";
    case BlockCompressionMethod::TuckerPreview:
      return "TuckerPreview";
  }
  return "DenseFallback";
}

BlockCompressionMethod blockCompressionMethodFromName(const std::string& name) {
  if (name == "MatrixSVD") {
    return BlockCompressionMethod::MatrixSVD;
  }
  if (name == "TuckerPreview") {
    return BlockCompressionMethod::TuckerPreview;
  }
  return BlockCompressionMethod::DenseFallback;
}

std::size_t CompressedAdaptiveBlockSDF::blockCount() const {
  return blocks.size();
}

std::size_t CompressedAdaptiveBlockSDF::originalMemoryBytes() const {
  std::size_t bytes = 0;
  for (const CompressedSDFBlock& block : blocks) {
    bytes += block.original_bytes;
  }
  return bytes;
}

std::size_t CompressedAdaptiveBlockSDF::compressedMemoryBytes() const {
  std::size_t bytes = sizeof(CompressedAdaptiveBlockSDF);
  for (const CompressedSDFBlock& block : blocks) {
    bytes += sizeof(CompressedSDFBlock);
    if (block.method == BlockCompressionMethod::MatrixSVD) {
      bytes += block.svd.U.size() * sizeof(double);
      bytes += block.svd.S.size() * sizeof(double);
      bytes += block.svd.Vt.size() * sizeof(double);
    } else {
      bytes += block.dense_phi.size() * sizeof(double);
    }
  }
  return bytes;
}

double CompressedAdaptiveBlockSDF::compressionRatio() const {
  const std::size_t compressed = compressedMemoryBytes();
  if (compressed == 0) {
    return 1.0;
  }
  return static_cast<double>(originalMemoryBytes()) /
         static_cast<double>(compressed);
}

double compressedBlockGridValue(
    const CompressedSDFBlock& block,
    int ix,
    int iy,
    int iz) {
  if (block.nx <= 0 || block.ny <= 0 || block.nz <= 0) {
    return 0.0;
  }
  ix = std::clamp(ix, 0, block.nx - 1);
  iy = std::clamp(iy, 0, block.ny - 1);
  iz = std::clamp(iz, 0, block.nz - 1);

  if (block.method == BlockCompressionMethod::MatrixSVD) {
    const int col = iy * block.nz + iz;
    if (block.svd.rank <= 0 || block.svd.rows != block.nx ||
        block.svd.cols != block.ny * block.nz ||
        block.svd.U.size() !=
            static_cast<std::size_t>(block.svd.rows) *
                static_cast<std::size_t>(block.svd.rank) ||
        block.svd.S.size() != static_cast<std::size_t>(block.svd.rank) ||
        block.svd.Vt.size() !=
            static_cast<std::size_t>(block.svd.rank) *
                static_cast<std::size_t>(block.svd.cols)) {
      return 0.0;
    }
    double value = 0.0;
    for (int r = 0; r < block.svd.rank; ++r) {
      value += block.svd.U[matrixIndex(ix, r, block.svd.rank)] *
               block.svd.S[static_cast<std::size_t>(r)] *
               block.svd.Vt[matrixIndex(r, col, block.svd.cols)];
    }
    return std::isfinite(value) ? value : 0.0;
  }

  const std::size_t expected = denseValueCount(block);
  if (block.dense_phi.size() != expected) {
    return 0.0;
  }
  const double value = block.dense_phi[denseIndex(ix, iy, iz, block.nx, block.ny)];
  return std::isfinite(value) ? value : 0.0;
}

}  // namespace adasdf

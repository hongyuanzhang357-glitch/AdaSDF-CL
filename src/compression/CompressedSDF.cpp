#include "adasdf/compression/CompressedSDF.h"

#include <stdexcept>

namespace adasdf {

std::size_t CompressedSDF::memoryFootprintBytes() const {
  std::size_t bytes = sizeof(*this);
  for (const LowRankBlock& block : blocks_) {
    bytes += sizeof(block);
    bytes += block.payload.basis.size() * sizeof(Scalar);
    bytes += block.payload.coefficients.size() * sizeof(Scalar);
  }
  return bytes;
}

CompressedSDF CompressedSDF::load(const std::filesystem::path&) {
  throw std::runtime_error(
      "CompressedSDF::load is not a public v0.2 entry point; use SDFBinReader::read.");
}

void CompressedSDF::save(const std::filesystem::path&) const {
  throw std::runtime_error("CompressedSDF::save is not implemented in v0.2.");
}

std::vector<Scalar> CompressedSDF::decompressBlock(BlockId) const {
  throw std::runtime_error(
      "CompressedSDF::decompressBlock is not implemented in v0.2.");
}

}  // namespace adasdf

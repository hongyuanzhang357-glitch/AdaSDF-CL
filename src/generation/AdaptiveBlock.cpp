#include "adasdf/generation/AdaptiveBlock.h"

namespace adasdf {

std::size_t AdaptiveSDFBlockSet::blockCount() const {
  return blocks.size();
}

std::size_t AdaptiveSDFBlockSet::memoryFootprintBytes() const {
  std::size_t bytes = sizeof(AdaptiveSDFBlockSet);
  for (const AdaptiveSDFBlock& block : blocks) {
    bytes += sizeof(AdaptiveSDFBlock);
    bytes += block.phi.size() * sizeof(double);
  }
  return bytes;
}

}  // namespace adasdf

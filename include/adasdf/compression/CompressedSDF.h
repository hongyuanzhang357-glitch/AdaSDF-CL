#pragma once

#include <filesystem>
#include <vector>

#include "adasdf/compression/LowRankBlock.h"

namespace adasdf {

class CompressedSDF {
 public:
  const std::vector<LowRankBlock>& blocks() const {
    return blocks_;
  }

  std::vector<LowRankBlock>& mutableBlocks() {
    return blocks_;
  }

  std::size_t memoryFootprintBytes() const;

  // TODO: connect to sdf::ModelIO::loadBinary and sdf::SDFModelPackage.
  static CompressedSDF load(const std::filesystem::path& path);

  // TODO: connect to sdf::ModelIO::saveBinary and preserve format versioning.
  void save(const std::filesystem::path& path) const;

  // TODO: connect to sdf::LowRankReconstruction for dense block expansion.
  std::vector<Scalar> decompressBlock(BlockId block_id) const;

 private:
  std::vector<LowRankBlock> blocks_;
};

}  // namespace adasdf

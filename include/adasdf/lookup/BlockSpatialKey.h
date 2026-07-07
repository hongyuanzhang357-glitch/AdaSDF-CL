#pragma once

#include <cstddef>
#include <cstdint>

#include "adasdf/lookup/MortonKey.h"

namespace adasdf {

struct BlockSpatialKey {
  Int3 coord;
  int level = 0;
  std::uint64_t morton = 0;

  bool operator==(const BlockSpatialKey& other) const;
  bool operator<(const BlockSpatialKey& other) const;
};

struct BlockSpatialKeyHash {
  std::size_t operator()(const BlockSpatialKey& key) const noexcept;
};

BlockSpatialKey makeBlockSpatialKey(const Int3& coord, int level);

}  // namespace adasdf

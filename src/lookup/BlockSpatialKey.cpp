#include "adasdf/lookup/BlockSpatialKey.h"

#include <cstdint>
#include <functional>

namespace adasdf {
namespace {

std::uint32_t bias(int value) {
  return static_cast<std::uint32_t>(
      static_cast<std::int64_t>(value) + 1048576ll);
}

}  // namespace

bool BlockSpatialKey::operator==(const BlockSpatialKey& other) const {
  return coord.x == other.coord.x && coord.y == other.coord.y &&
         coord.z == other.coord.z && level == other.level &&
         morton == other.morton;
}

bool BlockSpatialKey::operator<(const BlockSpatialKey& other) const {
  if (level != other.level) {
    return level < other.level;
  }
  if (morton != other.morton) {
    return morton < other.morton;
  }
  if (coord.x != other.coord.x) {
    return coord.x < other.coord.x;
  }
  if (coord.y != other.coord.y) {
    return coord.y < other.coord.y;
  }
  return coord.z < other.coord.z;
}

std::size_t BlockSpatialKeyHash::operator()(
    const BlockSpatialKey& key) const noexcept {
  std::size_t seed = std::hash<std::uint64_t>{}(key.morton);
  seed ^= std::hash<int>{}(key.level) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  seed ^= std::hash<int>{}(key.coord.x) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  seed ^= std::hash<int>{}(key.coord.y) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  seed ^= std::hash<int>{}(key.coord.z) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  return seed;
}

BlockSpatialKey makeBlockSpatialKey(const Int3& coord, int level) {
  BlockSpatialKey key;
  key.coord = coord;
  key.level = level;
  key.morton = MortonKey::encode3D(
      bias(coord.x),
      bias(coord.y),
      bias(coord.z));
  return key;
}

}  // namespace adasdf

#include "adasdf/lookup/MortonKey.h"

namespace adasdf {
namespace {

std::uint64_t splitBy3(std::uint32_t value) {
  std::uint64_t x = value & 0x1fffffu;
  x = (x | (x << 32)) & 0x1f00000000ffffull;
  x = (x | (x << 16)) & 0x1f0000ff0000ffull;
  x = (x | (x << 8)) & 0x100f00f00f00f00full;
  x = (x | (x << 4)) & 0x10c30c30c30c30c3ull;
  x = (x | (x << 2)) & 0x1249249249249249ull;
  return x;
}

std::uint32_t compactBy3(std::uint64_t value) {
  std::uint64_t x = value & 0x1249249249249249ull;
  x = (x ^ (x >> 2)) & 0x10c30c30c30c30c3ull;
  x = (x ^ (x >> 4)) & 0x100f00f00f00f00full;
  x = (x ^ (x >> 8)) & 0x1f0000ff0000ffull;
  x = (x ^ (x >> 16)) & 0x1f00000000ffffull;
  x = (x ^ (x >> 32)) & 0x1fffffull;
  return static_cast<std::uint32_t>(x);
}

}  // namespace

std::uint64_t MortonKey::encode3D(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t z) {
  return splitBy3(x) | (splitBy3(y) << 1) | (splitBy3(z) << 2);
}

Int3 MortonKey::decode3D(std::uint64_t key) {
  return {
      static_cast<int>(compactBy3(key)),
      static_cast<int>(compactBy3(key >> 1)),
      static_cast<int>(compactBy3(key >> 2))};
}

}  // namespace adasdf

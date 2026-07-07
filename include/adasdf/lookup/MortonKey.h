#pragma once

#include <cstdint>

namespace adasdf {

struct Int3 {
  int x = 0;
  int y = 0;
  int z = 0;
};

struct MortonKey64 {
  std::uint64_t value = 0;
};

class MortonKey {
 public:
  static std::uint64_t encode3D(
      std::uint32_t x,
      std::uint32_t y,
      std::uint32_t z);
  static Int3 decode3D(std::uint64_t key);
};

}  // namespace adasdf

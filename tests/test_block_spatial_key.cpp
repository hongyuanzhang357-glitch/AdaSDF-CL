#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <unordered_map>

int main() {
  try {
    const adasdf::BlockSpatialKey a =
        adasdf::makeBlockSpatialKey({1, 2, 3}, 4);
    const adasdf::BlockSpatialKey b =
        adasdf::makeBlockSpatialKey({1, 2, 3}, 4);
    const adasdf::BlockSpatialKey c =
        adasdf::makeBlockSpatialKey({1, 2, 4}, 4);
    if (!(a == b) || a == c) {
      std::cerr << "spatial key equality failed\n";
      return 1;
    }
    std::unordered_map<
        adasdf::BlockSpatialKey,
        int,
        adasdf::BlockSpatialKeyHash>
        map;
    map[a] = 7;
    if (map[b] != 7 || map.find(c) != map.end()) {
      std::cerr << "spatial key hash failed\n";
      return 1;
    }
    std::cout << "block spatial key passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_block_spatial_key failed: " << exc.what() << "\n";
    return 1;
  }
}

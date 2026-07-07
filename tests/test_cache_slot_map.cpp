#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <vector>

namespace {

adasdf::ActiveExpandedBlock makeBlock(int id, double min_x, double max_x) {
  adasdf::ActiveExpandedBlock block;
  block.block_id = id;
  block.level = 1;
  block.bounds = {{min_x, 0.0, 0.0}, {max_x, 1.0, 1.0}, true};
  block.nx = 2;
  block.ny = 2;
  block.nz = 2;
  block.origin = block.bounds.min;
  block.spacing = {max_x - min_x, 1.0, 1.0};
  block.phi.assign(8, 0.0);
  return block;
}

}  // namespace

int main() {
  try {
    const std::vector<adasdf::ActiveExpandedBlock> blocks = {
        makeBlock(10, 0.0, 0.5),
        makeBlock(11, 0.5, 1.0)};
    adasdf::CacheSlotMap map;
    map.rebuildFromActiveBlocks(blocks);
    if (map.blockIdToSlot(10) != 0 || map.blockIdToSlot(11) != 1 ||
        map.pointToSlot({0.75, 0.5, 0.5}) != 1 ||
        !map.containsBlock(10)) {
      std::cerr << "cache slot map lookup failed\n";
      return 1;
    }
    std::cout << "cache slot map passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_cache_slot_map failed: " << exc.what() << "\n";
    return 1;
  }
}

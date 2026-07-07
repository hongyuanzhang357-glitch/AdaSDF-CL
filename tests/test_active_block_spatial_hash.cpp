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
    adasdf::BlockLookupIndexOptions options;
    options.mode = adasdf::BlockLookupMode::SpatialHash;
    options.allow_linear_fallback = true;
    adasdf::ActiveBlockSpatialHash hash;
    if (!hash.build(blocks, {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true}, options)) {
      std::cerr << "active spatial hash build failed\n";
      return 1;
    }
    if (hash.findActiveBlockContainingPoint({0.25, 0.5, 0.5}).block_id != 10 ||
        hash.findActiveBlockContainingPoint({0.75, 0.5, 0.5}).block_id != 11 ||
        hash.findCacheSlotForPoint({0.75, 0.5, 0.5}) != 1) {
      std::cerr << "active spatial hash lookup failed\n";
      return 1;
    }
    std::cout << "active block spatial hash passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_spatial_hash failed: " << exc.what()
              << "\n";
    return 1;
  }
}

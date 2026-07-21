#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(
    int id,
    int level,
    const adasdf::AABB& bounds) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = id;
  block.octree_node_id = id;
  block.level = level;
  block.bounds = bounds;
  block.nx = 2;
  block.ny = 2;
  block.nz = 2;
  block.origin = bounds.min;
  block.spacing = bounds.max - bounds.min;
  block.phi.assign(8, static_cast<double>(id));
  return block;
}

}  // namespace

int main() {
  adasdf::AdaptiveSDFBlockSet blocks;
  blocks.global_bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  int id = 0;
  for (int z = 0; z < 2; ++z) {
    for (int y = 0; y < 2; ++y) {
      for (int x = 0; x < 2; ++x) {
        const adasdf::AABB box{
            {0.5 * x, 0.5 * y, 0.5 * z},
            {0.5 * (x + 1), 0.5 * (y + 1), 0.5 * (z + 1)},
            true};
        blocks.blocks.push_back(makeBlock(id++, 1, box));
      }
    }
  }

  adasdf::AdaptiveBlockSDFModel model(blocks);
  adasdf::NarrowBandBrickIndex index;
  if (!index.build(model) || index.stats().block_count != 8) {
    std::cerr << "failed to build narrow-band brick index\n";
    return 1;
  }
  adasdf::NarrowBandBrickLookupStats stats;
  const auto* record = index.find({0.75, 0.25, 0.25}, &stats);
  if (record == nullptr || record->brick_id != 1 ||
      stats.blocks_checked != 1 || stats.block_lookup_miss_count != 0) {
    std::cerr << "brick index did not locate expected brick\n";
    return 1;
  }
  std::cout << "narrow-band brick index passed\n";
  return 0;
}


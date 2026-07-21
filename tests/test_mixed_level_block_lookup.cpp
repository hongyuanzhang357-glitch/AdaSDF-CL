#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <vector>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(
    int id,
    int level,
    const adasdf::AABB& bounds) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = id;
  block.level = level;
  block.bounds = bounds;
  block.nx = 2;
  block.ny = 2;
  block.nz = 2;
  block.origin = bounds.min;
  block.spacing = bounds.max - bounds.min;
  return block;
}

}  // namespace

int main() {
  try {
    const std::vector<adasdf::AdaptiveSDFBlock> blocks = {
        makeBlock(
            1,
            1,
            {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true}),
        makeBlock(
            2,
            2,
            {{0.25, 0.25, 0.25}, {0.75, 0.75, 0.75}, true})};
    const adasdf::AABB domain{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
    for (adasdf::BlockLookupMode mode :
         {adasdf::BlockLookupMode::LinearScan,
          adasdf::BlockLookupMode::SpatialHash,
          adasdf::BlockLookupMode::MortonSorted}) {
      adasdf::BlockLookupIndexOptions options;
      options.mode = mode;
      options.allow_linear_fallback = true;
      adasdf::BlockLookupIndex lookup;
      if (!lookup.buildFromAdaptiveBlocks(blocks, domain, options)) {
        std::cerr << "lookup build failed\n";
        return 1;
      }
      const auto center =
          lookup.findBlockContainingPoint({0.5, 0.5, 0.5});
      if (!center.found || center.block_id != 2) {
        std::cerr << "lookup did not choose deepest mixed-level block\n";
        return 1;
      }
      const auto outer =
          lookup.findBlockContainingPoint({0.1, 0.1, 0.1});
      if (!outer.found || outer.block_id != 1) {
        std::cerr << "lookup did not choose coarse fallback block\n";
        return 1;
      }
    }
    std::cout << "mixed level block lookup passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mixed_level_block_lookup failed: " << exc.what()
              << "\n";
    return 1;
  }
}

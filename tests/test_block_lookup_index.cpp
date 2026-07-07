#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <vector>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(int id, double min_x, double max_x) {
  adasdf::AdaptiveSDFBlock block;
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
    const std::vector<adasdf::AdaptiveSDFBlock> blocks = {
        makeBlock(10, 0.0, 0.5),
        makeBlock(11, 0.5, 1.0)};
    const adasdf::AABB domain{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
    adasdf::BlockLookupIndexOptions linear_options;
    linear_options.mode = adasdf::BlockLookupMode::LinearScan;
    adasdf::BlockLookupIndex linear;
    if (!linear.buildFromAdaptiveBlocks(blocks, domain, linear_options)) {
      std::cerr << "linear build failed\n";
      return 1;
    }
    for (adasdf::BlockLookupMode mode :
         {adasdf::BlockLookupMode::SpatialHash,
          adasdf::BlockLookupMode::MortonSorted}) {
      adasdf::BlockLookupIndexOptions options;
      options.mode = mode;
      options.allow_linear_fallback = true;
      adasdf::BlockLookupIndex index;
      if (!index.buildFromAdaptiveBlocks(blocks, domain, options)) {
        std::cerr << "index build failed\n";
        return 1;
      }
      for (const adasdf::Vector3 p :
           {adasdf::Vector3{0.25, 0.5, 0.5},
            adasdf::Vector3{0.75, 0.5, 0.5}}) {
        const auto expected = linear.findBlockContainingPoint(p);
        const auto actual = index.findBlockContainingPoint(p);
        if (!actual.found || actual.block_id != expected.block_id) {
          std::cerr << "lookup index mismatch\n";
          return 1;
        }
      }
    }
    std::cout << "block lookup index passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_block_lookup_index failed: " << exc.what() << "\n";
    return 1;
  }
}

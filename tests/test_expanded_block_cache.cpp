#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

namespace {

adasdf::ActiveExpandedBlock makeBlock(int id) {
  adasdf::ActiveExpandedBlock block;
  block.block_id = id;
  block.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  block.nx = 2;
  block.ny = 2;
  block.nz = 2;
  block.origin = {0.0, 0.0, 0.0};
  block.spacing = {1.0, 1.0, 1.0};
  block.phi.assign(8, static_cast<double>(id));
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::ExpandedBlockCacheOptions options;
    options.max_blocks = 1;
    adasdf::ExpandedBlockCache cache(options);
    cache.put(makeBlock(1));
    if (!cache.contains(1) || !cache.get(1)) {
      std::cerr << "cache did not store block\n";
      return 1;
    }
    cache.put(makeBlock(2));
    if (cache.contains(1) || !cache.contains(2) || cache.get(42) != nullptr) {
      std::cerr << "cache LRU behavior failed\n";
      return 1;
    }
    const auto stats = cache.stats();
    if (stats.block_count != 1 || stats.eviction_count == 0 ||
        stats.hit_count == 0 || stats.miss_count == 0) {
      std::cerr << "cache stats failed\n";
      return 1;
    }
    std::cout << "expanded block cache passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_expanded_block_cache failed: " << exc.what() << "\n";
    return 1;
  }
}

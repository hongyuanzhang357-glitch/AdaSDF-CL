#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    adasdf::ExpandedBlockCache cache;
    adasdf::BlockExpansionManager manager(&cache);
    const auto first = manager.ensureBlocksExpanded(model, {0});
    const auto second = manager.ensureBlocksExpanded(model, {0});
    if (!first.success || !second.success ||
        first.stats.expanded_block_count != 1 ||
        second.stats.cache_hit_count != 1 ||
        !cache.contains(0) || cache.stats().memory_bytes == 0) {
      std::cerr << "block expansion manager failed\n";
      return 1;
    }
    std::cout << "block expansion manager passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_block_expansion_manager failed: " << exc.what() << "\n";
    return 1;
  }
}

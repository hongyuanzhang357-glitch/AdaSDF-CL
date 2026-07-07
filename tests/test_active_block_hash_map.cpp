#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::ActiveBlockHashMap map;
    map.rebuild({{10, 0, 0, true}, {42, 3, 0, true}});
    if (map.findCacheSlot(10) != 0 || map.findCacheSlot(42) != 3 ||
        map.findCacheSlot(99) != -1) {
      std::cerr << "active block hash map lookup failed\n";
      return 1;
    }
    const auto stats = map.stats();
    if (stats.active_count != 2 || stats.hit_count != 2 ||
        stats.miss_count != 1 || !(stats.hit_rate > 0.0)) {
      std::cerr << "active block hash map stats failed\n";
      return 1;
    }
    std::cout << "active block hash map passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_hash_map failed: " << exc.what() << "\n";
    return 1;
  }
}

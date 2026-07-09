#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  adasdf::CellCornerCache cache;
  const adasdf::QuantizedPointKey key{4, 5, 6, 1};
  double phi = 0.0;
  if (cache.findCorner(key, &phi)) {
    std::cerr << "empty corner cache should miss\n";
    return 1;
  }
  cache.insertCorner(key, 0.375);
  if (!cache.findCorner(key, &phi) || std::abs(phi - 0.375) > 1e-15) {
    std::cerr << "corner cache did not reuse inserted phi\n";
    return 1;
  }
  const adasdf::CellCornerCacheStats stats = cache.snapshotStats();
  if (stats.hit_count != 1 || stats.miss_count != 1 ||
      stats.insert_count != 1 || stats.entry_count != 1) {
    std::cerr << "unexpected corner cache stats\n";
    return 1;
  }
  cache.clearBlockLocal();
  if (cache.snapshotStats().entry_count != 0) {
    std::cerr << "corner cache clear did not remove entries\n";
    return 1;
  }
  return 0;
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  adasdf::SDFSampleCache cache;
  const adasdf::QuantizedPointKey key{1, 2, 3, 4};
  adasdf::CachedSDFSample sample;
  if (cache.find(key, &sample)) {
    std::cerr << "empty cache should miss\n";
    return 1;
  }
  sample.phi = -0.125;
  sample.sign_known = true;
  sample.sign = -1;
  sample.nearest_triangle_id = 7;
  cache.insert(key, sample);

  adasdf::CachedSDFSample found;
  if (!cache.find(key, &found) || std::abs(found.phi + 0.125) > 1e-15 ||
      found.sign != -1 || found.nearest_triangle_id != 7) {
    std::cerr << "cache hit returned the wrong sample\n";
    return 1;
  }
  const adasdf::SDFSampleCacheStats stats = cache.snapshotStats();
  if (stats.lookup_count != 2 || stats.hit_count != 1 ||
      stats.miss_count != 1 || stats.insert_count != 1 ||
      stats.distance_query_saved != 1 || stats.sign_query_saved != 1 ||
      stats.entry_count != 1) {
    std::cerr << "unexpected SDF sample cache stats\n";
    return 1;
  }
  return 0;
}

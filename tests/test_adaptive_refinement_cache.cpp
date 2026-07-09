#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  adasdf::BuildCacheOptions options;
  options.sample_cache = adasdf::BuildCacheScope::Block;
  options.distance_cache = true;
  options.sign_cache = true;
  adasdf::AdaptiveRefinementCache cache(options);
  adasdf::QuantizationOptions quantization;
  quantization.origin = {0.0, 0.0, 0.0};
  quantization.spacing = 0.25;
  quantization.epsilon = 1e-9;
  cache.setQuantization(quantization);
  cache.beginBlock(12);

  adasdf::CachedSDFSample sample;
  if (cache.findSample({0.25, 0.0, 0.0}, 3, &sample)) {
    std::cerr << "adaptive refinement cache should miss before insert\n";
    return 1;
  }
  sample.phi = 0.25;
  sample.sign_known = true;
  sample.sign = 1;
  cache.insertSample({0.25, 0.0, 0.0}, 3, sample);

  adasdf::CachedSDFSample found;
  if (!cache.findSample({0.25 + 1e-13, 0.0, 0.0}, 3, &found) ||
      std::abs(found.phi - 0.25) > 1e-15) {
    std::cerr << "adaptive refinement cache failed to reuse sample\n";
    return 1;
  }
  const adasdf::BuildCacheStats stats = cache.snapshotStats();
  if (!stats.sample_cache_enabled ||
      stats.sample_cache_scope != adasdf::BuildCacheScope::Block ||
      stats.sample_cache_hits != 1 || stats.sample_cache_misses != 1 ||
      stats.distance_queries_saved != 1 || stats.sign_queries_saved != 1) {
    std::cerr << "unexpected adaptive refinement cache stats\n";
    return 1;
  }

  cache.beginBlock(13);
  if (cache.findSample({0.25, 0.0, 0.0}, 3, &found)) {
    std::cerr << "block-local cache should clear between blocks\n";
    return 1;
  }
  return 0;
}

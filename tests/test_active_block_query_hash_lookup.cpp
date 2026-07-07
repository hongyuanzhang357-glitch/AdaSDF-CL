#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    const auto samples = active_block_tests::makeSamples();
    adasdf::ExpandedBlockCache linear_cache;
    adasdf::ExpandedBlockCache hash_cache;
    adasdf::ActiveBlockQueryOptions linear_options;
    linear_options.cache_lookup_mode = adasdf::BlockLookupMode::LinearScan;
    adasdf::ActiveBlockQueryOptions hash_options;
    hash_options.cache_lookup_mode = adasdf::BlockLookupMode::SpatialHash;
    hash_options.allow_linear_fallback = true;

    const auto linear = adasdf::ActiveBlockQuery::query(
        model, samples, {0}, linear_cache, linear_options);
    const auto hashed = adasdf::ActiveBlockQuery::query(
        model, samples, {0}, hash_cache, hash_options);
    if (!linear.success || !hashed.success ||
        linear.samples.size() != hashed.samples.size()) {
      std::cerr << "active block hash query failed\n";
      return 1;
    }
    for (std::size_t i = 0; i < linear.samples.size(); ++i) {
      if (linear.samples[i].sample_id != hashed.samples[i].sample_id ||
          std::abs(linear.samples[i].phi - hashed.samples[i].phi) > 1.0e-12 ||
          linear.sample_sources[i] != hashed.sample_sources[i]) {
        std::cerr << "active block hash query mismatch\n";
        return 1;
      }
    }
    if (hashed.stats.lookup_stats.query_count == 0) {
      std::cerr << "active block hash query stats missing\n";
      return 1;
    }
    std::cout << "active block query hash lookup passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_query_hash_lookup failed: "
              << exc.what() << "\n";
    return 1;
  }
}

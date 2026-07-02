#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    const auto samples = active_block_tests::makeSamples();
    adasdf::ExpandedBlockCache cache;
    adasdf::ActiveBlockQueryOptions options;
    options.compute_normals = true;
    options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
    const auto result =
        adasdf::ActiveBlockQuery::query(model, samples, {0}, cache, options);
    if (!result.success || !result.colliding ||
        result.stats.cache_query_count != 1 ||
        result.stats.fallback_query_count != 1 ||
        result.samples.size() != 2 ||
        result.sample_sources.size() != 2 ||
        result.sample_sources[0] != "cache" ||
        result.sample_sources[1] != "fallback") {
      std::cerr << "active block query failed\n";
      return 1;
    }
    std::cout << "active block query passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_query failed: " << exc.what() << "\n";
    return 1;
  }
}

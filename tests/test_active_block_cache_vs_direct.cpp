#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    const auto samples = active_block_tests::makeSamples();
    adasdf::ExpandedBlockCache cache;
    adasdf::ActiveBlockQueryOptions active_options;
    active_options.fallback_to_model_query = false;
    const auto active =
        adasdf::ActiveBlockQuery::query(model, samples, {0, 1}, cache, active_options);
    adasdf::SparseSDFQueryOptions direct_options;
    const auto direct =
        adasdf::SparseSDFQuery::query(model, samples, direct_options);
    if (!active.success || !direct.success ||
        active.samples.size() != direct.samples.size()) {
      std::cerr << "query setup failed\n";
      return 1;
    }
    for (std::size_t i = 0; i < active.samples.size(); ++i) {
      if (std::abs(active.samples[i].effective_phi -
                   direct.samples[i].effective_phi) > 1.0e-12) {
        std::cerr << "active cache and direct query differ\n";
        return 1;
      }
    }
    std::cout << "active block cache vs direct passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_cache_vs_direct failed: "
              << exc.what() << "\n";
    return 1;
  }
}

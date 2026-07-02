#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  try {
    const auto model = active_block_tests::makeCompressedModel();
    const auto samples = active_block_tests::makeSamples();
    adasdf::ActiveBlockSelectionOptions options;
    options.threshold = 0.0;
    options.include_neighbor_blocks = false;
    const auto result =
        adasdf::ActiveBlockSelector::select(model, samples, options);
    if (!result.success || result.block_ids.size() != 1 ||
        result.block_ids[0] != 0 ||
        result.candidate_sample_count != 1) {
      std::cerr << "unexpected active block selection result\n";
      return 1;
    }
    std::cout << "active block selector passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_active_block_selector failed: " << exc.what() << "\n";
    return 1;
  }
}

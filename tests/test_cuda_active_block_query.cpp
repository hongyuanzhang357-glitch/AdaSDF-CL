#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  if (!adasdf::CudaActiveBlockQuery::isAvailable()) {
    std::cout << "SKIP: CUDA active block query unavailable\n";
    return 0;
  }
  adasdf::CudaActiveBlockQueryOptions options;
  options.threshold = 1.0;
  options.selection_band = 0.1;
  options.include_neighbors = true;
  options.output_mode = adasdf::CudaActiveBlockOutputMode::PhiOnly;

  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  const adasdf::CudaActiveBlockQueryResult result =
      adasdf::CudaActiveBlockQuery::query(
          model, active_block_tests::makeSamples(), options);
  if (!result.success || !result.cuda_available ||
      result.samples.size() != active_block_tests::makeSamples().size() ||
      result.stats.gpu_query_count == 0) {
    std::cerr << "CUDA active block query failed: " << result.error_message
              << "\n";
    return 1;
  }
  for (const auto& sample : result.samples) {
    if (!std::isfinite(sample.phi) || !std::isfinite(sample.effective_phi)) {
      std::cerr << "CUDA active block query produced non-finite output\n";
      return 1;
    }
  }
  std::cout << "CUDA active block query passed\n";
  return 0;
}

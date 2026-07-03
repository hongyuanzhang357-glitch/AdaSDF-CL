#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  if (!adasdf::CudaActiveBlockQuery::isAvailable()) {
    std::cout << "SKIP: CUDA active block phi+normal unavailable\n";
    return 0;
  }
  adasdf::CudaActiveBlockQueryOptions options;
  options.threshold = 1.0;
  options.selection_band = 0.1;
  options.include_neighbors = true;
  options.compute_normals = true;
  options.output_mode = adasdf::CudaActiveBlockOutputMode::PhiAndNormal;

  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  const adasdf::CudaActiveBlockQueryResult result =
      adasdf::CudaActiveBlockQuery::query(
          model, active_block_tests::makeSamples(), options);
  if (!result.success || result.samples.empty()) {
    std::cerr << "CUDA active block phi+normal query failed\n";
    return 1;
  }
  for (const auto& sample : result.samples) {
    if (!sample.has_normal ||
        !std::isfinite(sample.normal.x) ||
        !std::isfinite(sample.normal.y) ||
        !std::isfinite(sample.normal.z)) {
      std::cerr << "CUDA active block normal is missing or non-finite\n";
      return 1;
    }
  }
  std::cout << "CUDA active block phi+normal passed\n";
  return 0;
}

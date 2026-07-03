#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  if (!adasdf::CudaActiveBlockQuery::isAvailable()) {
    std::cout << "SKIP: CUDA active block alignment unavailable\n";
    return 0;
  }

  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  const adasdf::CollisionSampleSet samples = active_block_tests::makeSamples();

  adasdf::ActiveBlockSelectionOptions cpu_selection;
  cpu_selection.threshold = 1.0;
  cpu_selection.selection_band = 0.1;
  cpu_selection.include_neighbor_blocks = true;
  adasdf::ActiveBlockQueryOptions cpu_options;
  cpu_options.threshold = 1.0;
  cpu_options.compute_normals = false;
  cpu_options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
  adasdf::ExpandedBlockCache cpu_cache;
  const adasdf::ActiveBlockQueryResult cpu_result =
      adasdf::ActiveBlockQuery::queryWithSelection(
          model, samples, cpu_cache, cpu_selection, cpu_options);

  adasdf::CudaActiveBlockQueryOptions cuda_options;
  cuda_options.threshold = 1.0;
  cuda_options.selection_band = 0.1;
  cuda_options.include_neighbors = true;
  cuda_options.compute_normals = false;
  const adasdf::CudaActiveBlockQueryResult cuda_result =
      adasdf::CudaActiveBlockQuery::query(model, samples, cuda_options);

  if (!cpu_result.success || !cuda_result.success ||
      cpu_result.samples.size() != cuda_result.samples.size()) {
    std::cerr << "CPU/CUDA active block query setup failed\n";
    return 1;
  }
  for (std::size_t i = 0; i < cpu_result.samples.size(); ++i) {
    const double error =
        std::abs(cpu_result.samples[i].phi - cuda_result.samples[i].phi);
    if (error > 1.0e-9) {
      std::cerr << "CPU/CUDA phi mismatch: " << error << "\n";
      return 1;
    }
  }
  std::cout << "CUDA active block alignment passed\n";
  return 0;
}

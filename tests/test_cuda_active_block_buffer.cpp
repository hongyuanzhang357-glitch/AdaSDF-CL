#include <adasdf/adasdf.h>

#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  adasdf::ExpandedBlockCache cache;
  adasdf::BlockExpansionManager manager(&cache);
  const adasdf::BlockExpansionResult expansion =
      manager.ensureBlocksExpanded(model, {0, 1});
  if (!expansion.success) {
    std::cerr << "block expansion failed: " << expansion.error_message << "\n";
    return 1;
  }

  adasdf::CudaActiveBlockBuffer buffer;
  const adasdf::CudaActiveBlockUploadStats upload =
      buffer.uploadFromCache(cache, {0, 1});
  if (!buffer.isAvailable()) {
    if (upload.success || upload.error_message.empty()) {
      std::cerr << "CUDA unavailable upload should fail with an error\n";
      return 1;
    }
    std::cout << "SKIP: CUDA active block buffer unavailable\n";
    return 0;
  }

  if (!upload.success || buffer.blockCount() != 2 ||
      buffer.valueCount() == 0 || buffer.deviceMemoryBytes() == 0) {
    std::cerr << "CUDA active block buffer upload failed\n";
    return 1;
  }
  std::cout << "CUDA active block buffer passed\n";
  return 0;
}

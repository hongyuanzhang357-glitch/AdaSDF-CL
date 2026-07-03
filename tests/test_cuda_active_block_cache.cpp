#include <adasdf/adasdf.h>

#include <iostream>

#include "active_block_test_helpers.h"

int main() {
  adasdf::CompressedAdaptiveBlockSDFModel model =
      active_block_tests::makeCompressedModel();
  adasdf::CudaActiveBlockCache cache;
  adasdf::BlockExpansionManager manager(&cache.cpuCache());
  const adasdf::BlockExpansionResult expansion =
      manager.ensureBlocksExpanded(model, {0, 1});
  if (!expansion.success) {
    std::cerr << "block expansion failed: " << expansion.error_message << "\n";
    return 1;
  }

  const adasdf::CudaActiveBlockUploadStats upload =
      cache.uploadActiveBlocks({0, 1});
  const adasdf::CudaActiveBlockCacheStats stats = cache.stats();
  if (!cache.isCudaAvailable()) {
    if (upload.success || upload.error_message.empty()) {
      std::cerr << "CUDA unavailable cache upload should fail with an error\n";
      return 1;
    }
    std::cout << "SKIP: CUDA active block cache unavailable\n";
    return 0;
  }

  if (!upload.success || stats.gpu_block_count != 2 ||
      stats.gpu_memory_bytes == 0 || stats.upload_count != 1) {
    std::cerr << "CUDA active block cache upload failed\n";
    return 1;
  }
  cache.clear();
  if (cache.stats().gpu_block_count != 0) {
    std::cerr << "CUDA active block cache clear failed\n";
    return 1;
  }
  std::cout << "CUDA active block cache passed\n";
  return 0;
}

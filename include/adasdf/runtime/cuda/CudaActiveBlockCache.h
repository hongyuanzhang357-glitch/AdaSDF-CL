#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/lookup/MortonKey.h"
#include "adasdf/runtime/ExpandedBlockCache.h"
#include "adasdf/runtime/cuda/CudaActiveBlockBuffer.h"

namespace adasdf {

struct CudaActiveBlockCacheOptions {
  std::size_t cpu_max_blocks = 64;
  std::size_t cpu_max_memory_bytes = 256ull * 1024ull * 1024ull;
  std::size_t gpu_max_memory_bytes = 256ull * 1024ull * 1024ull;
  bool enable_lru_eviction = true;
  bool reupload_each_query = true;
  bool enable_gpu_resident_reuse = false;
};

struct CudaActiveBlockCacheStats {
  ExpandedBlockCacheStats cpu_cache_stats;
  std::size_t gpu_block_count = 0;
  std::size_t gpu_value_count = 0;
  std::size_t gpu_memory_bytes = 0;
  std::size_t upload_count = 0;
  double last_upload_time_ms = 0.0;
  std::vector<std::string> warnings;
};

struct FlatBlockLookupTable {
  std::vector<std::uint64_t> keys;
  std::vector<int> block_ids;
  std::vector<int> cache_slots;
  std::vector<AABB> bounds;
};

class CudaActiveBlockCache {
 public:
  explicit CudaActiveBlockCache(
      const CudaActiveBlockCacheOptions& options =
          CudaActiveBlockCacheOptions{});

  bool isCudaAvailable() const;

  ExpandedBlockCache& cpuCache();
  const ExpandedBlockCache& cpuCache() const;
  CudaActiveBlockBuffer& gpuBuffer();
  const CudaActiveBlockBuffer& gpuBuffer() const;

  CudaActiveBlockUploadStats uploadActiveBlocks(
      const std::vector<int>& active_block_ids);

  CudaActiveBlockCacheStats stats() const;
  FlatBlockLookupTable flatLookupTable() const;
  void clear();

 private:
  CudaActiveBlockCacheOptions options_;
  ExpandedBlockCache cpu_cache_;
  CudaActiveBlockBuffer gpu_buffer_;
  std::size_t upload_count_ = 0;
  double last_upload_time_ms_ = 0.0;
  std::vector<std::string> warnings_;
};

}  // namespace adasdf

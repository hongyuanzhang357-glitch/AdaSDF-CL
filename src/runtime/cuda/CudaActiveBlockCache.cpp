#include "adasdf/runtime/cuda/CudaActiveBlockCache.h"

#include <algorithm>
#include <utility>

namespace adasdf {

namespace {

ExpandedBlockCacheOptions cpuOptionsFromCudaOptions(
    const CudaActiveBlockCacheOptions& options) {
  ExpandedBlockCacheOptions cpu_options;
  cpu_options.max_blocks = options.cpu_max_blocks;
  cpu_options.max_memory_bytes = options.cpu_max_memory_bytes;
  cpu_options.enable_lru = options.enable_lru_eviction;
  return cpu_options;
}

}  // namespace

CudaActiveBlockCache::CudaActiveBlockCache(
    const CudaActiveBlockCacheOptions& options)
    : options_(options), cpu_cache_(cpuOptionsFromCudaOptions(options)) {}

bool CudaActiveBlockCache::isCudaAvailable() const {
  return gpu_buffer_.isAvailable();
}

ExpandedBlockCache& CudaActiveBlockCache::cpuCache() {
  return cpu_cache_;
}

const ExpandedBlockCache& CudaActiveBlockCache::cpuCache() const {
  return cpu_cache_;
}

CudaActiveBlockBuffer& CudaActiveBlockCache::gpuBuffer() {
  return gpu_buffer_;
}

const CudaActiveBlockBuffer& CudaActiveBlockCache::gpuBuffer() const {
  return gpu_buffer_;
}

CudaActiveBlockUploadStats CudaActiveBlockCache::uploadActiveBlocks(
    const std::vector<int>& active_block_ids) {
  const CudaActiveBlockUploadStats upload =
      gpu_buffer_.uploadFromCache(cpu_cache_, active_block_ids);
  warnings_.insert(
      warnings_.end(),
      upload.warnings.begin(),
      upload.warnings.end());
  if (upload.success) {
    ++upload_count_;
    last_upload_time_ms_ = upload.upload_time_ms;
  }
  return upload;
}

CudaActiveBlockCacheStats CudaActiveBlockCache::stats() const {
  CudaActiveBlockCacheStats out;
  out.cpu_cache_stats = cpu_cache_.stats();
  out.gpu_block_count = gpu_buffer_.blockCount();
  out.gpu_value_count = gpu_buffer_.valueCount();
  out.gpu_memory_bytes = gpu_buffer_.deviceMemoryBytes();
  out.upload_count = upload_count_;
  out.last_upload_time_ms = last_upload_time_ms_;
  out.warnings = warnings_;
  if (options_.enable_gpu_resident_reuse) {
    out.warnings.push_back(
        "GPU resident reuse is declared but not implemented in v1.11; "
        "active blocks are reuploaded as a baseline path.");
  }
  return out;
}

FlatBlockLookupTable CudaActiveBlockCache::flatLookupTable() const {
  FlatBlockLookupTable table;
  const std::vector<ActiveExpandedBlock> blocks = cpu_cache_.residentBlocks();
  table.keys.reserve(blocks.size());
  table.block_ids.reserve(blocks.size());
  table.cache_slots.reserve(blocks.size());
  table.bounds.reserve(blocks.size());
  for (std::size_t slot = 0; slot < blocks.size(); ++slot) {
    const ActiveExpandedBlock& block = blocks[slot];
    table.keys.push_back(MortonKey::encode3D(
        static_cast<std::uint32_t>(std::max(0, block.block_id)),
        static_cast<std::uint32_t>(std::max(0, block.level)),
        0u));
    table.block_ids.push_back(block.block_id);
    table.cache_slots.push_back(static_cast<int>(slot));
    table.bounds.push_back(block.bounds);
  }
  return table;
}

void CudaActiveBlockCache::clear() {
  cpu_cache_.clear();
  gpu_buffer_.clear();
  upload_count_ = 0;
  last_upload_time_ms_ = 0.0;
  warnings_.clear();
}

}  // namespace adasdf

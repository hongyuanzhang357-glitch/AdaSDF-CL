#include "adasdf/runtime/cuda/CudaActiveBlockBuffer.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

#include "adasdf/backend/CudaQueryBackend.h"

namespace adasdf {

#ifndef ADASDF_CL_HAS_CUDA_BACKEND
#define ADASDF_CL_HAS_CUDA_BACKEND 0
#endif

#if ADASDF_CL_HAS_CUDA_BACKEND
namespace cuda_detail {
void* createActiveBlockBufferOnCuda();
bool uploadActiveBlockBufferOnCuda(
    void* handle,
    const std::vector<CudaActiveBlockMetadata>& metadata,
    const std::vector<double>& values,
    std::size_t* device_memory_bytes,
    double* upload_time_ms,
    std::string* error_message);
void releaseActiveBlockBufferOnCuda(void* handle);
std::size_t activeBlockBufferBlockCountOnCuda(void* handle);
std::size_t activeBlockBufferValueCountOnCuda(void* handle);
std::size_t activeBlockBufferDeviceMemoryBytesOnCuda(void* handle);
}  // namespace cuda_detail
#endif

namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

CudaActiveBlockMetadata metadataFromBlock(
    const ActiveExpandedBlock& block,
    std::size_t value_offset) {
  CudaActiveBlockMetadata metadata;
  metadata.block_id = block.block_id;
  metadata.level = block.level;
  metadata.min_x = block.bounds.min.x;
  metadata.min_y = block.bounds.min.y;
  metadata.min_z = block.bounds.min.z;
  metadata.max_x = block.bounds.max.x;
  metadata.max_y = block.bounds.max.y;
  metadata.max_z = block.bounds.max.z;
  metadata.origin_x = block.origin.x;
  metadata.origin_y = block.origin.y;
  metadata.origin_z = block.origin.z;
  metadata.spacing_x = block.spacing.x;
  metadata.spacing_y = block.spacing.y;
  metadata.spacing_z = block.spacing.z;
  metadata.nx = block.nx;
  metadata.ny = block.ny;
  metadata.nz = block.nz;
  metadata.value_offset = value_offset > static_cast<std::size_t>(
                                       std::numeric_limits<int>::max())
      ? std::numeric_limits<int>::max()
      : static_cast<int>(value_offset);
  metadata.value_count = block.phi.size() > static_cast<std::size_t>(
                                            std::numeric_limits<int>::max())
      ? std::numeric_limits<int>::max()
      : static_cast<int>(block.phi.size());
  metadata.near_surface = block.near_surface ? 1 : 0;
  return metadata;
}

}  // namespace

CudaActiveBlockBuffer::~CudaActiveBlockBuffer() {
  clear();
}

CudaActiveBlockBuffer::CudaActiveBlockBuffer(
    CudaActiveBlockBuffer&& other) noexcept {
  *this = std::move(other);
}

CudaActiveBlockBuffer& CudaActiveBlockBuffer::operator=(
    CudaActiveBlockBuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  clear();
  device_handle_ = other.device_handle_;
  block_count_ = other.block_count_;
  value_count_ = other.value_count_;
  device_memory_bytes_ = other.device_memory_bytes_;
  other.device_handle_ = nullptr;
  other.block_count_ = 0;
  other.value_count_ = 0;
  other.device_memory_bytes_ = 0;
  return *this;
}

bool CudaActiveBlockBuffer::isAvailable() const {
#if ADASDF_CL_HAS_CUDA_BACKEND
  return CudaQueryBackend::isAvailable();
#else
  return false;
#endif
}

CudaActiveBlockUploadStats CudaActiveBlockBuffer::uploadFromCache(
    const ExpandedBlockCache& cache,
    const std::vector<int>& active_block_ids) {
  CudaActiveBlockUploadStats stats;
  const auto pack_start = Clock::now();
  std::vector<CudaActiveBlockMetadata> metadata;
  std::vector<double> values;
  metadata.reserve(active_block_ids.size());

  for (const int block_id : active_block_ids) {
    const ActiveExpandedBlock* block = cache.peek(block_id);
    if (block == nullptr) {
      stats.warnings.push_back(
          "active block " + std::to_string(block_id) +
          " was not resident in the CPU cache and was not uploaded.");
      continue;
    }
    if (!block->isValid()) {
      stats.warnings.push_back(
          "active block " + std::to_string(block_id) +
          " was invalid and was not uploaded.");
      continue;
    }
    metadata.push_back(metadataFromBlock(*block, values.size()));
    values.insert(values.end(), block->phi.begin(), block->phi.end());
  }

  stats.block_count = metadata.size();
  stats.value_count = values.size();
  stats.metadata_bytes = metadata.size() * sizeof(CudaActiveBlockMetadata);
  stats.value_bytes = values.size() * sizeof(double);
  stats.total_bytes = stats.metadata_bytes + stats.value_bytes;
  stats.pack_time_ms = elapsedMs(pack_start, Clock::now());

  clear();
  if (metadata.empty()) {
    stats.error_message = "no resident active blocks were available to upload.";
    return stats;
  }

#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!isAvailable()) {
    stats.error_message = "CUDA active block buffer unavailable at runtime.";
    return stats;
  }
  device_handle_ = cuda_detail::createActiveBlockBufferOnCuda();
  if (device_handle_ == nullptr) {
    stats.error_message = "failed to create CUDA active block buffer.";
    return stats;
  }
  std::string error_message;
  if (!cuda_detail::uploadActiveBlockBufferOnCuda(
          device_handle_,
          metadata,
          values,
          &device_memory_bytes_,
          &stats.upload_time_ms,
          &error_message)) {
    stats.error_message = error_message.empty()
        ? "failed to upload CUDA active block buffer."
        : error_message;
    clear();
    return stats;
  }
  block_count_ = cuda_detail::activeBlockBufferBlockCountOnCuda(device_handle_);
  value_count_ = cuda_detail::activeBlockBufferValueCountOnCuda(device_handle_);
  device_memory_bytes_ =
      cuda_detail::activeBlockBufferDeviceMemoryBytesOnCuda(device_handle_);
  stats.block_count = block_count_;
  stats.value_count = value_count_;
  stats.total_bytes = device_memory_bytes_;
  stats.success = true;
  return stats;
#else
  stats.error_message =
      "CUDA active block buffer was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.";
  return stats;
#endif
}

void CudaActiveBlockBuffer::clear() {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (device_handle_ != nullptr) {
    cuda_detail::releaseActiveBlockBufferOnCuda(device_handle_);
  }
#endif
  device_handle_ = nullptr;
  block_count_ = 0;
  value_count_ = 0;
  device_memory_bytes_ = 0;
}

std::size_t CudaActiveBlockBuffer::blockCount() const {
  return block_count_;
}

std::size_t CudaActiveBlockBuffer::valueCount() const {
  return value_count_;
}

std::size_t CudaActiveBlockBuffer::deviceMemoryBytes() const {
  return device_memory_bytes_;
}

void* CudaActiveBlockBuffer::deviceHandle() const {
  return device_handle_;
}

}  // namespace adasdf

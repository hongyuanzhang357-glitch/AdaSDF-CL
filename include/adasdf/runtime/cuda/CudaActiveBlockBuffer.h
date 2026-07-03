#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"
#include "adasdf/runtime/ExpandedBlockCache.h"

namespace adasdf {

struct CudaActiveBlockMetadata {
  int block_id = -1;
  int level = 0;

  double min_x = 0.0;
  double min_y = 0.0;
  double min_z = 0.0;
  double max_x = 0.0;
  double max_y = 0.0;
  double max_z = 0.0;

  double origin_x = 0.0;
  double origin_y = 0.0;
  double origin_z = 0.0;
  double spacing_x = 0.0;
  double spacing_y = 0.0;
  double spacing_z = 0.0;

  int nx = 0;
  int ny = 0;
  int nz = 0;
  int value_offset = 0;
  int value_count = 0;
  int near_surface = 0;
};

struct CudaActiveBlockUploadStats {
  bool success = false;
  std::string error_message;
  std::size_t block_count = 0;
  std::size_t value_count = 0;
  std::size_t metadata_bytes = 0;
  std::size_t value_bytes = 0;
  std::size_t total_bytes = 0;
  double pack_time_ms = 0.0;
  double upload_time_ms = 0.0;
  std::vector<std::string> warnings;
};

struct CudaActiveBlockDeviceQueryResult {
  bool success = false;
  std::string error_message;
  std::vector<double> phi;
  std::vector<double> effective_phi;
  std::vector<int> block_ids;
  std::vector<int> fallback_needed;
  std::vector<int> colliding;
  std::vector<Vector3> normals;
  double sample_upload_time_ms = 0.0;
  double kernel_time_ms = 0.0;
  double download_time_ms = 0.0;
};

class CudaActiveBlockBuffer {
 public:
  CudaActiveBlockBuffer() = default;
  ~CudaActiveBlockBuffer();

  CudaActiveBlockBuffer(const CudaActiveBlockBuffer&) = delete;
  CudaActiveBlockBuffer& operator=(const CudaActiveBlockBuffer&) = delete;
  CudaActiveBlockBuffer(CudaActiveBlockBuffer&& other) noexcept;
  CudaActiveBlockBuffer& operator=(CudaActiveBlockBuffer&& other) noexcept;

  bool isAvailable() const;

  CudaActiveBlockUploadStats uploadFromCache(
      const ExpandedBlockCache& cache,
      const std::vector<int>& active_block_ids);

  void clear();

  std::size_t blockCount() const;
  std::size_t valueCount() const;
  std::size_t deviceMemoryBytes() const;
  void* deviceHandle() const;

 private:
  void* device_handle_ = nullptr;
  std::size_t block_count_ = 0;
  std::size_t value_count_ = 0;
  std::size_t device_memory_bytes_ = 0;
};

}  // namespace adasdf

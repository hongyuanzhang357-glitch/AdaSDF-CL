#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/geometry/AnalyticSDFModel.h"
#include "adasdf/geometry/DemoAdaptiveSDFModel.h"
#include "adasdf/query/ExpandedSDF.h"
#include "adasdf/query/BatchQuery.h"

namespace adasdf {

class CudaQueryWorkspace {
 public:
  CudaQueryWorkspace() = default;
  ~CudaQueryWorkspace();

  CudaQueryWorkspace(const CudaQueryWorkspace&) = delete;
  CudaQueryWorkspace& operator=(const CudaQueryWorkspace&) = delete;
  CudaQueryWorkspace(CudaQueryWorkspace&& other) noexcept;
  CudaQueryWorkspace& operator=(CudaQueryWorkspace&& other) noexcept;

  bool allocate(std::size_t capacity, bool need_normals);
  bool ensureCapacity(std::size_t capacity, bool need_normals);

  bool uploadPoints(const std::vector<Vector3>& points);
  BatchQueryOutput downloadResults(std::size_t count, bool need_normals);

  void release();

  std::size_t capacity() const;
  std::size_t deviceMemoryBytes() const;
  std::size_t allocationCount() const;
  bool lastEnsureReused() const;

 private:
  friend class CudaResidentExpandedSDF;

  void* workspace_handle_ = nullptr;
  std::size_t capacity_ = 0;
  std::size_t device_memory_bytes_ = 0;
  std::size_t allocation_count_ = 0;
  bool last_ensure_reused_ = false;
  bool need_normals_ = false;
};

class CudaQueryBackend {
 public:
  static bool isAvailable();

  static BatchQueryOutput queryAnalyticBox(
      const AnalyticSDFModel& model,
      const BatchQueryInput& input);

  static BatchQueryOutput queryDemoAdaptiveBox(
      const DemoAdaptiveSDFModel& model,
      const BatchQueryInput& input);
};

class CudaResidentExpandedSDF {
 public:
  CudaResidentExpandedSDF() = default;
  ~CudaResidentExpandedSDF();

  CudaResidentExpandedSDF(const CudaResidentExpandedSDF&) = delete;
  CudaResidentExpandedSDF& operator=(const CudaResidentExpandedSDF&) = delete;
  CudaResidentExpandedSDF(CudaResidentExpandedSDF&& other) noexcept;
  CudaResidentExpandedSDF& operator=(CudaResidentExpandedSDF&& other) noexcept;

  bool upload(const ExpandedSDF& expanded);
  bool isValid() const;
  std::size_t deviceMemoryBytes() const;

  BatchQueryOutput queryBatch(const std::vector<Vector3>& points);
  BatchQueryOutput queryBatch(
      const std::vector<Vector3>& points,
      bool phi_only,
      CudaQueryWorkspace* workspace,
      BatchQueryTiming* timing);
  BatchQueryOutput queryBatch(
      const std::vector<Vector3>& points,
      QueryOutputMode output_mode,
      CudaQueryWorkspace* workspace,
      BatchQueryTiming* timing);
  bool queryBatchInto(
      const std::vector<Vector3>& points,
      QueryOutputMode output_mode,
      CudaQueryWorkspace* workspace,
      BatchQueryOutput* output,
      BatchQueryTiming* timing,
      bool download_results = true);
  void release();

  double lastKernelMs() const {
    return last_kernel_ms_;
  }

  double lastQueryMs() const {
    return last_query_ms_;
  }

  const BatchQueryTiming& lastTiming() const {
    return last_timing_;
  }

 private:
  void* device_handle_ = nullptr;
  std::size_t device_memory_bytes_ = 0;
  double last_kernel_ms_ = 0.0;
  double last_query_ms_ = 0.0;
  BatchQueryTiming last_timing_;
};

}  // namespace adasdf

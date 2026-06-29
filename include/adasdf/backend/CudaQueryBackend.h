#pragma once

#include "adasdf/geometry/AnalyticSDFModel.h"
#include "adasdf/geometry/DemoAdaptiveSDFModel.h"
#include "adasdf/query/ExpandedSDF.h"
#include "adasdf/query/BatchQuery.h"

namespace adasdf {

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
  void release();

  double lastKernelMs() const {
    return last_kernel_ms_;
  }

  double lastQueryMs() const {
    return last_query_ms_;
  }

 private:
  void* device_handle_ = nullptr;
  std::size_t device_memory_bytes_ = 0;
  double last_kernel_ms_ = 0.0;
  double last_query_ms_ = 0.0;
};

}  // namespace adasdf

#include "adasdf/backend/CudaQueryBackend.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace adasdf {

#ifndef ADASDF_CL_HAS_CUDA_BACKEND
#define ADASDF_CL_HAS_CUDA_BACKEND 0
#endif

#if ADASDF_CL_HAS_CUDA_BACKEND
namespace cuda_detail {
bool isRuntimeAvailable();
BatchQueryOutput queryAnalyticBoxOnCuda(
    const Vector3& center,
    const Vector3& half_extent,
    const BatchQueryInput& input);
void* uploadExpandedSDFToCuda(
    const ExpandedSDF& expanded,
    std::size_t* device_memory_bytes);
BatchQueryOutput queryExpandedSDFOnCuda(
    void* device_handle,
    const std::vector<Vector3>& points,
    bool phi_only,
    void* workspace_handle,
    BatchQueryTiming* timing);
void* createQueryWorkspaceOnCuda();
bool ensureQueryWorkspaceOnCuda(
    void* workspace_handle,
    std::size_t capacity,
    bool need_normals,
    std::size_t* device_memory_bytes);
bool uploadQueryWorkspacePointsOnCuda(
    void* workspace_handle,
    const std::vector<Vector3>& points,
    BatchQueryTiming* timing);
BatchQueryOutput downloadQueryWorkspaceResultsOnCuda(
    void* workspace_handle,
    std::size_t count,
    bool need_normals,
    BatchQueryTiming* timing);
void releaseQueryWorkspaceOnCuda(void* workspace_handle);
std::size_t queryWorkspaceCapacityOnCuda(void* workspace_handle);
std::size_t queryWorkspaceDeviceMemoryBytesOnCuda(void* workspace_handle);
void releaseExpandedSDFOnCuda(void* device_handle);
}  // namespace cuda_detail
#endif

CudaQueryWorkspace::~CudaQueryWorkspace() {
  release();
}

CudaQueryWorkspace::CudaQueryWorkspace(CudaQueryWorkspace&& other) noexcept {
  *this = std::move(other);
}

CudaQueryWorkspace& CudaQueryWorkspace::operator=(
    CudaQueryWorkspace&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  release();
  workspace_handle_ = other.workspace_handle_;
  capacity_ = other.capacity_;
  device_memory_bytes_ = other.device_memory_bytes_;
  need_normals_ = other.need_normals_;
  other.workspace_handle_ = nullptr;
  other.capacity_ = 0;
  other.device_memory_bytes_ = 0;
  other.need_normals_ = false;
  return *this;
}

bool CudaQueryWorkspace::allocate(std::size_t capacity, bool need_normals) {
  release();
  return ensureCapacity(capacity, need_normals);
}

bool CudaQueryWorkspace::ensureCapacity(
    std::size_t capacity,
    bool need_normals) {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!CudaQueryBackend::isAvailable()) {
    return false;
  }
  if (workspace_handle_ == nullptr) {
    workspace_handle_ = cuda_detail::createQueryWorkspaceOnCuda();
  }
  if (workspace_handle_ == nullptr) {
    return false;
  }
  if (!cuda_detail::ensureQueryWorkspaceOnCuda(
          workspace_handle_,
          capacity,
          need_normals,
          &device_memory_bytes_)) {
    return false;
  }
  capacity_ = cuda_detail::queryWorkspaceCapacityOnCuda(workspace_handle_);
  device_memory_bytes_ =
      cuda_detail::queryWorkspaceDeviceMemoryBytesOnCuda(workspace_handle_);
  need_normals_ = need_normals;
  return true;
#else
  (void)capacity;
  (void)need_normals;
  return false;
#endif
}

bool CudaQueryWorkspace::uploadPoints(const std::vector<Vector3>& points) {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (workspace_handle_ == nullptr || points.size() > capacity_) {
    return false;
  }
  return cuda_detail::uploadQueryWorkspacePointsOnCuda(
      workspace_handle_,
      points,
      nullptr);
#else
  (void)points;
  return false;
#endif
}

BatchQueryOutput CudaQueryWorkspace::downloadResults(
    std::size_t count,
    bool need_normals) {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (workspace_handle_ == nullptr || count > capacity_) {
    throw std::runtime_error("CudaQueryWorkspace download before allocation.");
  }
  return cuda_detail::downloadQueryWorkspaceResultsOnCuda(
      workspace_handle_,
      count,
      need_normals,
      nullptr);
#else
  (void)count;
  (void)need_normals;
  throw std::runtime_error(
      "CUDA query workspace was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.");
#endif
}

void CudaQueryWorkspace::release() {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (workspace_handle_ != nullptr) {
    cuda_detail::releaseQueryWorkspaceOnCuda(workspace_handle_);
  }
#endif
  workspace_handle_ = nullptr;
  capacity_ = 0;
  device_memory_bytes_ = 0;
  need_normals_ = false;
}

std::size_t CudaQueryWorkspace::capacity() const {
  return capacity_;
}

std::size_t CudaQueryWorkspace::deviceMemoryBytes() const {
  return device_memory_bytes_;
}

bool CudaQueryBackend::isAvailable() {
#if ADASDF_CL_HAS_CUDA_BACKEND
  return cuda_detail::isRuntimeAvailable();
#else
  return false;
#endif
}

BatchQueryOutput CudaQueryBackend::queryAnalyticBox(
    const AnalyticSDFModel& model,
    const BatchQueryInput& input) {
  if (model.shapeType() != AnalyticShapeType::Box) {
    throw std::runtime_error("CUDA query backend currently supports box SDF only.");
  }
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!isAvailable()) {
    throw std::runtime_error("CUDA query backend is not available at runtime.");
  }
  return cuda_detail::queryAnalyticBoxOnCuda(
      model.center(),
      model.halfExtent(),
      input);
#else
  (void)input;
  throw std::runtime_error(
      "CUDA query backend was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.");
#endif
}

BatchQueryOutput CudaQueryBackend::queryDemoAdaptiveBox(
    const DemoAdaptiveSDFModel& model,
    const BatchQueryInput& input) {
  if (model.shapeName() != "box") {
    throw std::runtime_error(
        "CUDA query backend currently supports demo adaptive box SDF only.");
  }
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!isAvailable()) {
    throw std::runtime_error("CUDA query backend is not available at runtime.");
  }
  return cuda_detail::queryAnalyticBoxOnCuda(
      model.center(),
      model.halfExtent(),
      input);
#else
  (void)input;
  throw std::runtime_error(
      "CUDA query backend was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.");
#endif
}

CudaResidentExpandedSDF::~CudaResidentExpandedSDF() {
  release();
}

CudaResidentExpandedSDF::CudaResidentExpandedSDF(
    CudaResidentExpandedSDF&& other) noexcept {
  *this = std::move(other);
}

CudaResidentExpandedSDF& CudaResidentExpandedSDF::operator=(
    CudaResidentExpandedSDF&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  release();
  device_handle_ = other.device_handle_;
  device_memory_bytes_ = other.device_memory_bytes_;
  last_kernel_ms_ = other.last_kernel_ms_;
  last_query_ms_ = other.last_query_ms_;
  last_timing_ = other.last_timing_;
  other.device_handle_ = nullptr;
  other.device_memory_bytes_ = 0;
  other.last_kernel_ms_ = 0.0;
  other.last_query_ms_ = 0.0;
  other.last_timing_ = {};
  return *this;
}

bool CudaResidentExpandedSDF::upload(const ExpandedSDF& expanded) {
  if (!expanded.isValid()) {
    throw std::runtime_error("CudaResidentExpandedSDF upload requires valid expanded SDF data.");
  }
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (!CudaQueryBackend::isAvailable()) {
    return false;
  }
  release();
  device_handle_ =
      cuda_detail::uploadExpandedSDFToCuda(expanded, &device_memory_bytes_);
  return device_handle_ != nullptr;
#else
  (void)expanded;
  return false;
#endif
}

bool CudaResidentExpandedSDF::isValid() const {
  return device_handle_ != nullptr;
}

std::size_t CudaResidentExpandedSDF::deviceMemoryBytes() const {
  return device_memory_bytes_;
}

BatchQueryOutput CudaResidentExpandedSDF::queryBatch(
    const std::vector<Vector3>& points) {
  return queryBatch(points, false, nullptr, nullptr);
}

BatchQueryOutput CudaResidentExpandedSDF::queryBatch(
    const std::vector<Vector3>& points,
    bool phi_only,
    CudaQueryWorkspace* workspace,
    BatchQueryTiming* timing) {
  if (!isValid()) {
    throw std::runtime_error("CudaResidentExpandedSDF query before successful upload.");
  }
#if ADASDF_CL_HAS_CUDA_BACKEND
  const auto t0 = std::chrono::steady_clock::now();
  BatchQueryTiming local_timing;
  if (workspace != nullptr && !workspace->ensureCapacity(points.size(), !phi_only)) {
    throw std::runtime_error("CudaQueryWorkspace allocation failed.");
  }
  BatchQueryOutput output =
      cuda_detail::queryExpandedSDFOnCuda(
          device_handle_,
          points,
          phi_only,
          workspace != nullptr ? workspace->workspace_handle_ : nullptr,
          &local_timing);
  last_query_ms_ = local_timing.total_ms;
  if (last_query_ms_ <= 0.0) {
    last_query_ms_ = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - t0)
                         .count();
    local_timing.total_ms = last_query_ms_;
  }
  last_kernel_ms_ = local_timing.kernel_ms;
  if (workspace != nullptr) {
    workspace->capacity_ =
        cuda_detail::queryWorkspaceCapacityOnCuda(workspace->workspace_handle_);
    workspace->device_memory_bytes_ =
        cuda_detail::queryWorkspaceDeviceMemoryBytesOnCuda(workspace->workspace_handle_);
    workspace->need_normals_ = !phi_only;
  }
  last_timing_ = local_timing;
  if (timing != nullptr) {
    *timing = local_timing;
  }
  return output;
#else
  (void)points;
  (void)phi_only;
  (void)workspace;
  (void)timing;
  throw std::runtime_error(
      "CUDA query backend was not compiled. Configure with "
      "ADASDF_CL_ENABLE_CUDA=ON on a machine with a CUDA toolkit.");
#endif
}

void CudaResidentExpandedSDF::release() {
#if ADASDF_CL_HAS_CUDA_BACKEND
  if (device_handle_ != nullptr) {
    cuda_detail::releaseExpandedSDFOnCuda(device_handle_);
  }
#endif
  device_handle_ = nullptr;
  device_memory_bytes_ = 0;
  last_kernel_ms_ = 0.0;
  last_query_ms_ = 0.0;
  last_timing_ = {};
}

}  // namespace adasdf

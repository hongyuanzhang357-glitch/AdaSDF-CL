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
    double* kernel_ms);
void releaseExpandedSDFOnCuda(void* device_handle);
}  // namespace cuda_detail
#endif

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
  other.device_handle_ = nullptr;
  other.device_memory_bytes_ = 0;
  other.last_kernel_ms_ = 0.0;
  other.last_query_ms_ = 0.0;
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
  if (!isValid()) {
    throw std::runtime_error("CudaResidentExpandedSDF query before successful upload.");
  }
#if ADASDF_CL_HAS_CUDA_BACKEND
  const auto t0 = std::chrono::steady_clock::now();
  BatchQueryOutput output =
      cuda_detail::queryExpandedSDFOnCuda(
          device_handle_,
          points,
          &last_kernel_ms_);
  last_query_ms_ = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - t0)
                       .count();
  return output;
#else
  (void)points;
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
}

}  // namespace adasdf

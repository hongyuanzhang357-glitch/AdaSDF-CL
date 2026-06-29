#include "adasdf/backend/CudaQueryBackend.h"

#include <stdexcept>

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

}  // namespace adasdf

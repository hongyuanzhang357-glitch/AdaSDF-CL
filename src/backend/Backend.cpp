#include "adasdf/backend/Backend.h"

#include <memory>

#include "adasdf/backend/CpuBackend.h"
#include "adasdf/backend/GpuBackend.h"

namespace adasdf {

std::unique_ptr<Backend> makeBackend(BackendType type) {
  if (type == BackendType::CUDA) {
    return std::make_unique<GpuBackend>();
  }
  return std::make_unique<CpuBackend>();
}

}  // namespace adasdf

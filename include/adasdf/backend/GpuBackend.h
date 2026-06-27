#pragma once

#include "adasdf/backend/Backend.h"

namespace adasdf {

class GpuBackend final : public Backend {
 public:
  BackendType type() const override {
    return BackendType::CUDA;
  }

  // TODO: connect to existing sdf::CudaGlobalDenseSDF and CUDA preflight checks.
  bool available() const override;

  std::string name() const override {
    return "CUDA";
  }
};

}  // namespace adasdf

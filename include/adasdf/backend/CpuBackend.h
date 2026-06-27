#pragma once

#include "adasdf/backend/Backend.h"

namespace adasdf {

class CpuBackend final : public Backend {
 public:
  BackendType type() const override {
    return BackendType::CPU;
  }

  bool available() const override {
    return true;
  }

  std::string name() const override {
    return "CPU";
  }
};

}  // namespace adasdf

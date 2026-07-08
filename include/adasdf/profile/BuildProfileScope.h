#pragma once

#include <chrono>

namespace adasdf {

class BuildProfileScope {
 public:
  explicit BuildProfileScope(double* target_ms);
  ~BuildProfileScope();

 private:
  double* target_ms_ = nullptr;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace adasdf

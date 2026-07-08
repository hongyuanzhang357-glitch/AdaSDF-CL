#pragma once

#include <chrono>

namespace adasdf {

class TimeoutGuard {
 public:
  explicit TimeoutGuard(double max_seconds = 0.0);
  bool enabled() const;
  bool expired() const;
  double maxSeconds() const {
    return max_seconds_;
  }

 private:
  double max_seconds_ = 0.0;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace adasdf

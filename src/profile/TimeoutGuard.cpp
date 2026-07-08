#include "adasdf/profile/TimeoutGuard.h"

namespace adasdf {

TimeoutGuard::TimeoutGuard(double max_seconds)
    : max_seconds_(max_seconds), start_(std::chrono::steady_clock::now()) {}

bool TimeoutGuard::enabled() const {
  return max_seconds_ > 0.0;
}

bool TimeoutGuard::expired() const {
  if (!enabled()) {
    return false;
  }
  return std::chrono::duration<double>(
             std::chrono::steady_clock::now() - start_)
             .count() >= max_seconds_;
}

}  // namespace adasdf

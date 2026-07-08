#include "adasdf/profile/BuildProfileScope.h"

namespace adasdf {

BuildProfileScope::BuildProfileScope(double* target_ms)
    : target_ms_(target_ms), start_(std::chrono::steady_clock::now()) {}

BuildProfileScope::~BuildProfileScope() {
  if (!target_ms_) {
    return;
  }
  *target_ms_ += std::chrono::duration<double, std::milli>(
                     std::chrono::steady_clock::now() - start_)
                     .count();
}

}  // namespace adasdf

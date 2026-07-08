#include "adasdf/profile/BuildProfiler.h"

#include <utility>

namespace adasdf {

BuildProfiler::BuildProfiler(bool enabled) : enabled_(enabled) {
  start();
}

void BuildProfiler::start() {
  start_ = std::chrono::steady_clock::now();
}

void BuildProfiler::setToolName(std::string value) {
  tool_name = std::move(value);
}

void BuildProfiler::setInputPath(std::string value) {
  input_path = std::move(value);
}

void BuildProfiler::setOutputPath(std::string value) {
  output_path = std::move(value);
}

void BuildProfiler::addWarning(
    const std::string& code,
    const std::string& message) {
  warnings.push_back({code, message});
}

void BuildProfiler::finishCompleted() {
  status = "completed";
  status_code = ErrorCode::OK;
  success = true;
  timings.total_time_ms = elapsedMs();
}

void BuildProfiler::finishTimeout() {
  status = "timeout";
  status_code = ErrorCode::TIMEOUT;
  success = false;
  timings.total_time_ms = elapsedMs();
}

void BuildProfiler::finishFailed(ErrorCode code, const std::string& message) {
  status = "failed";
  status_code = code;
  success = false;
  error_message = message;
  timings.total_time_ms = elapsedMs();
}

double BuildProfiler::elapsedMs() const {
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start_)
      .count();
}

}  // namespace adasdf

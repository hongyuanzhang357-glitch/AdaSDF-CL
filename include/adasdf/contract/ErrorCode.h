#pragma once

#include <string>

namespace adasdf {

enum class ErrorCode {
  OK,
  NO_COLLISION,
  NO_CANDIDATE,
  OUT_OF_DOMAIN,
  INVALID_MODEL,
  INVALID_ARGUMENT,
  IO_ERROR,
  TIMEOUT,
  UNSUPPORTED_BACKEND,
  CUDA_UNAVAILABLE,
  SCHEMA_VALIDATION_FAILED,
  INTERNAL_ERROR
};

const char* toString(ErrorCode code);
bool parseErrorCode(const std::string& text, ErrorCode* code);

}  // namespace adasdf

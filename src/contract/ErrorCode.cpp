#include "adasdf/contract/ErrorCode.h"

namespace adasdf {

const char* toString(ErrorCode code) {
  switch (code) {
    case ErrorCode::OK:
      return "ADASDF_OK";
    case ErrorCode::NO_COLLISION:
      return "ADASDF_NO_COLLISION";
    case ErrorCode::NO_CANDIDATE:
      return "ADASDF_NO_CANDIDATE";
    case ErrorCode::OUT_OF_DOMAIN:
      return "ADASDF_OUT_OF_DOMAIN";
    case ErrorCode::INVALID_MODEL:
      return "ADASDF_INVALID_MODEL";
    case ErrorCode::INVALID_ARGUMENT:
      return "ADASDF_INVALID_ARGUMENT";
    case ErrorCode::IO_ERROR:
      return "ADASDF_IO_ERROR";
    case ErrorCode::TIMEOUT:
      return "ADASDF_TIMEOUT";
    case ErrorCode::UNSUPPORTED_BACKEND:
      return "ADASDF_UNSUPPORTED_BACKEND";
    case ErrorCode::CUDA_UNAVAILABLE:
      return "ADASDF_CUDA_UNAVAILABLE";
    case ErrorCode::SCHEMA_VALIDATION_FAILED:
      return "ADASDF_SCHEMA_VALIDATION_FAILED";
    case ErrorCode::INTERNAL_ERROR:
      return "ADASDF_INTERNAL_ERROR";
  }
  return "ADASDF_INTERNAL_ERROR";
}

bool parseErrorCode(const std::string& text, ErrorCode* code) {
  if (!code) {
    return false;
  }
  const struct Entry {
    ErrorCode code;
    const char* text;
  } entries[] = {
      {ErrorCode::OK, "ADASDF_OK"},
      {ErrorCode::NO_COLLISION, "ADASDF_NO_COLLISION"},
      {ErrorCode::NO_CANDIDATE, "ADASDF_NO_CANDIDATE"},
      {ErrorCode::OUT_OF_DOMAIN, "ADASDF_OUT_OF_DOMAIN"},
      {ErrorCode::INVALID_MODEL, "ADASDF_INVALID_MODEL"},
      {ErrorCode::INVALID_ARGUMENT, "ADASDF_INVALID_ARGUMENT"},
      {ErrorCode::IO_ERROR, "ADASDF_IO_ERROR"},
      {ErrorCode::TIMEOUT, "ADASDF_TIMEOUT"},
      {ErrorCode::UNSUPPORTED_BACKEND, "ADASDF_UNSUPPORTED_BACKEND"},
      {ErrorCode::CUDA_UNAVAILABLE, "ADASDF_CUDA_UNAVAILABLE"},
      {ErrorCode::SCHEMA_VALIDATION_FAILED,
       "ADASDF_SCHEMA_VALIDATION_FAILED"},
      {ErrorCode::INTERNAL_ERROR, "ADASDF_INTERNAL_ERROR"}};
  for (const Entry& entry : entries) {
    if (text == entry.text) {
      *code = entry.code;
      return true;
    }
  }
  return false;
}

}  // namespace adasdf

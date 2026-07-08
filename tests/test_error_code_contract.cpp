#include <adasdf/adasdf.h>

#include <iostream>
#include <string>

int main() {
  const struct Entry {
    adasdf::ErrorCode code;
    const char* text;
  } entries[] = {
      {adasdf::ErrorCode::OK, "ADASDF_OK"},
      {adasdf::ErrorCode::NO_COLLISION, "ADASDF_NO_COLLISION"},
      {adasdf::ErrorCode::NO_CANDIDATE, "ADASDF_NO_CANDIDATE"},
      {adasdf::ErrorCode::OUT_OF_DOMAIN, "ADASDF_OUT_OF_DOMAIN"},
      {adasdf::ErrorCode::INVALID_MODEL, "ADASDF_INVALID_MODEL"},
      {adasdf::ErrorCode::INVALID_ARGUMENT, "ADASDF_INVALID_ARGUMENT"},
      {adasdf::ErrorCode::IO_ERROR, "ADASDF_IO_ERROR"},
      {adasdf::ErrorCode::TIMEOUT, "ADASDF_TIMEOUT"},
      {adasdf::ErrorCode::UNSUPPORTED_BACKEND,
       "ADASDF_UNSUPPORTED_BACKEND"},
      {adasdf::ErrorCode::CUDA_UNAVAILABLE, "ADASDF_CUDA_UNAVAILABLE"},
      {adasdf::ErrorCode::SCHEMA_VALIDATION_FAILED,
       "ADASDF_SCHEMA_VALIDATION_FAILED"},
      {adasdf::ErrorCode::INTERNAL_ERROR, "ADASDF_INTERNAL_ERROR"}};
  for (const Entry& entry : entries) {
    if (std::string(adasdf::toString(entry.code)) != entry.text) {
      std::cerr << "unexpected error code string\n";
      return 1;
    }
    adasdf::ErrorCode parsed = adasdf::ErrorCode::INTERNAL_ERROR;
    if (!adasdf::parseErrorCode(entry.text, &parsed) || parsed != entry.code) {
      std::cerr << "failed to parse error code\n";
      return 1;
    }
  }
  return 0;
}

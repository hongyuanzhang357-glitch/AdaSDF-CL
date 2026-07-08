#pragma once

#include <string>

namespace adasdf {

enum class JsonStatus {
  Ok,
  Warning,
  Error
};

const char* toString(JsonStatus status);
bool parseJsonStatus(const std::string& text, JsonStatus* status);

}  // namespace adasdf

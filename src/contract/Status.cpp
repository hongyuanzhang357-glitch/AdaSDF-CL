#include "adasdf/contract/Status.h"

namespace adasdf {

const char* toString(JsonStatus status) {
  switch (status) {
    case JsonStatus::Ok:
      return "ok";
    case JsonStatus::Warning:
      return "warning";
    case JsonStatus::Error:
      return "error";
  }
  return "error";
}

bool parseJsonStatus(const std::string& text, JsonStatus* status) {
  if (!status) {
    return false;
  }
  if (text == "ok") {
    *status = JsonStatus::Ok;
    return true;
  }
  if (text == "warning") {
    *status = JsonStatus::Warning;
    return true;
  }
  if (text == "error") {
    *status = JsonStatus::Error;
    return true;
  }
  return false;
}

}  // namespace adasdf

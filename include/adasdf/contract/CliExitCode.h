#pragma once

namespace adasdf {

enum class CliExitCode {
  Success = 0,
  InvalidArguments = 2,
  RuntimeError = 3,
  IoError = 4,
  Timeout = 124
};

inline int toInt(CliExitCode code) {
  return static_cast<int>(code);
}

}  // namespace adasdf

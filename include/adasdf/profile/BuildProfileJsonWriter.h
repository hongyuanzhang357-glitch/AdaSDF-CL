#pragma once

#include <string>

#include "adasdf/profile/BuildProfiler.h"

namespace adasdf {

class BuildProfileJsonWriter {
 public:
  static std::string toJson(const BuildProfiler& profiler);
  static bool write(const std::string& path, const BuildProfiler& profiler);
};

}  // namespace adasdf

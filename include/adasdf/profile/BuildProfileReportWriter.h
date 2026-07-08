#pragma once

#include <string>

#include "adasdf/profile/BuildProfiler.h"

namespace adasdf {

class BuildProfileReportWriter {
 public:
  static std::string toMarkdown(const BuildProfiler& profiler);
  static bool writeMarkdown(
      const std::string& path,
      const BuildProfiler& profiler);
};

}  // namespace adasdf

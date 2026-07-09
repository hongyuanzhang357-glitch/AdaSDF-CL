#pragma once

#include <string>

#include "adasdf/cache/BuildCacheStats.h"

namespace adasdf {

class BuildCacheReportWriter {
 public:
  static std::string toMarkdown(const BuildCacheStats& stats);
  static std::string csvHeader();
  static std::string csvRow(const BuildCacheStats& stats);
};

}  // namespace adasdf

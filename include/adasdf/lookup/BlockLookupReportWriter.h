#pragma once

#include <string>

#include "adasdf/lookup/BlockLookupDiagnostics.h"

namespace adasdf {

class BlockLookupReportWriter {
 public:
  static std::string csvHeader();
  static std::string markdownSummary(
      const std::string& case_id,
      const BlockLookupDiagnostics& diagnostics);
};

}  // namespace adasdf

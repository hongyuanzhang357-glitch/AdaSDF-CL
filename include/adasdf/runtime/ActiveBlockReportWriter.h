#pragma once

#include <string>

#include "adasdf/runtime/ActiveBlockQuery.h"
#include "adasdf/runtime/ActiveBlockSelector.h"
#include "adasdf/runtime/BlockExpansionManager.h"

namespace adasdf {

class ActiveBlockReportWriter {
 public:
  static std::string selectionToMarkdown(
      const ActiveBlockSelectionResult& result);
  static std::string selectionToJson(
      const ActiveBlockSelectionResult& result);
  static bool writeSelectionCSV(
      const std::string& path,
      const ActiveBlockSelectionResult& result,
      std::string* error_message = nullptr);

  static std::string queryToMarkdown(const ActiveBlockQueryResult& result);
  static std::string queryToJson(const ActiveBlockQueryResult& result);
  static bool writeQueryCSV(
      const std::string& path,
      const ActiveBlockQueryResult& result,
      std::string* error_message = nullptr);

  static std::string expansionToMarkdown(
      const BlockExpansionResult& result);
  static std::string expansionToJson(const BlockExpansionResult& result);
};

}  // namespace adasdf

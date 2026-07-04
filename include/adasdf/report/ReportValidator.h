#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/report/StrictReportSchema.h"

namespace adasdf {

struct ReportValidationResult {
  bool readable = false;
  bool valid = false;
  std::vector<std::string> errors;
  StrictReportRecord record;
};

class ReportValidator {
 public:
  static ReportValidationResult validateString(const std::string& text);
  static ReportValidationResult validateFile(const std::filesystem::path& path);
};

}  // namespace adasdf


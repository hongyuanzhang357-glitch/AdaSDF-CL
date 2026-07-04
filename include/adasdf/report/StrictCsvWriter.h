#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/report/StrictReportSchema.h"

namespace adasdf {

class StrictCsvWriter {
 public:
  static std::string csvField(const std::string& value);
  static std::string headerLine();
  static std::string rowLine(const StrictReportRecord& record);
  static bool writeSummaryCSV(
      const std::filesystem::path& path,
      const std::vector<StrictReportRecord>& records,
      std::string* error_message = nullptr);
};

}  // namespace adasdf


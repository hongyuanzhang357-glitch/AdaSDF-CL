#pragma once

#include <filesystem>
#include <string>

#include "adasdf/audit/MismatchCellForensics.h"

namespace adasdf {

class MismatchCellReportWriter {
 public:
  static std::string toJson(const MismatchCellForensicsResult& result);
  static std::string toCSV(const MismatchCellForensicsResult& result);
  static std::string toMarkdown(const MismatchCellForensicsResult& result);

  static bool writeJson(
      const std::filesystem::path& path,
      const MismatchCellForensicsResult& result);
  static bool writeCSV(
      const std::filesystem::path& path,
      const MismatchCellForensicsResult& result);
  static bool writeMarkdown(
      const std::filesystem::path& path,
      const MismatchCellForensicsResult& result);
};

}  // namespace adasdf

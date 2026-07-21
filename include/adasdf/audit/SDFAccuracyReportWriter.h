#pragma once

#include <filesystem>
#include <string>

#include "adasdf/audit/SDFAccuracyAudit.h"

namespace adasdf {

class SDFAccuracyReportWriter {
 public:
  static std::string toJson(const SDFAccuracyAuditResult& result);
  static std::string toMarkdown(const SDFAccuracyAuditResult& result);

  static bool writeJson(
      const std::filesystem::path& path,
      const SDFAccuracyAuditResult& result);
  static bool writeCSV(
      const std::filesystem::path& path,
      const SDFAccuracyAuditResult& result);
  static bool writeMismatchCSV(
      const std::filesystem::path& path,
      const SDFAccuracyAuditResult& result,
      std::size_t max_samples = 5000);
  static bool writeMarkdown(
      const std::filesystem::path& path,
      const SDFAccuracyAuditResult& result);
};

}  // namespace adasdf

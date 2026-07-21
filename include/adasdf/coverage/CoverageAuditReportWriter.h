#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/coverage/ContactBandCoverageAudit.h"
#include "adasdf/coverage/CoverageDrivenRefinement.h"

namespace adasdf {

struct CoverageAuditIteration {
  int iteration = 0;
  CoverageAuditResult audit;
  CoverageDrivenRefinementResult refinement;
};

struct CoverageAuditRunReport {
  std::string case_id;
  CoverageAuditOptions options;
  std::vector<CoverageAuditIteration> iterations;
};

class CoverageAuditReportWriter {
 public:
  static std::string toJson(const CoverageAuditResult& result);
  static std::string toMarkdown(const CoverageAuditResult& result);
  static std::string toJson(const CoverageAuditRunReport& report);
  static std::string toMarkdown(const CoverageAuditRunReport& report);

  static bool writeJson(
      const std::filesystem::path& path,
      const CoverageAuditResult& result);
  static bool writeMarkdown(
      const std::filesystem::path& path,
      const CoverageAuditResult& result);
  static bool writeJson(
      const std::filesystem::path& path,
      const CoverageAuditRunReport& report);
  static bool writeMarkdown(
      const std::filesystem::path& path,
      const CoverageAuditRunReport& report);
};

}  // namespace adasdf

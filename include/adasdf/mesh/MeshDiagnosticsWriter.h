#pragma once

#include <string>

#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/MeshCleanup.h"
#include "adasdf/mesh/MeshReadiness.h"

namespace adasdf {

class MeshDiagnosticsWriter {
 public:
  static std::string toMarkdown(const MeshDiagnosticsReport& report);
  static std::string toJson(const MeshDiagnosticsReport& report);

  static std::string readinessToMarkdown(
      const MeshReadinessReport& readiness);
  static std::string readinessToJson(const MeshReadinessReport& readiness);

  static std::string combinedMarkdown(
      const MeshDiagnosticsReport& diagnostics,
      const MeshReadinessReport& readiness);
  static std::string combinedJson(
      const MeshDiagnosticsReport& diagnostics,
      const MeshReadinessReport& readiness);

  static std::string cleanupToMarkdown(const MeshCleanupStats& stats);

  static std::string cleanupComparisonMarkdown(
      const MeshDiagnosticsReport& before_diag,
      const MeshReadinessReport& before_ready,
      const MeshCleanupStats& cleanup_stats,
      const MeshDiagnosticsReport& after_diag,
      const MeshReadinessReport& after_ready);

  static void writeMarkdown(
      const std::string& path,
      const MeshDiagnosticsReport& report);

  static void writeJson(
      const std::string& path,
      const MeshDiagnosticsReport& report);
};

}  // namespace adasdf

#pragma once

#include <string>

#include "adasdf/mesh/MeshDiagnostics.h"
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

  static void writeMarkdown(
      const std::string& path,
      const MeshDiagnosticsReport& report);

  static void writeJson(
      const std::string& path,
      const MeshDiagnosticsReport& report);
};

}  // namespace adasdf

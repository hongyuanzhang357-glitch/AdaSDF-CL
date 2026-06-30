#pragma once

#include <string>

#include "adasdf/mesh/MeshDiagnostics.h"

namespace adasdf {

class MeshDiagnosticsWriter {
 public:
  static std::string toMarkdown(const MeshDiagnosticsReport& report);
  static std::string toJson(const MeshDiagnosticsReport& report);

  static void writeMarkdown(
      const std::string& path,
      const MeshDiagnosticsReport& report);

  static void writeJson(
      const std::string& path,
      const MeshDiagnosticsReport& report);
};

}  // namespace adasdf

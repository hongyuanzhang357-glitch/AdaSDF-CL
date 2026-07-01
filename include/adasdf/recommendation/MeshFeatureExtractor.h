#pragma once

#include <string>

#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/MeshReadiness.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/recommendation/BuildRecommendationTypes.h"

namespace adasdf {

class MeshFeatureExtractor {
 public:
  static MeshFeatureSummary fromMesh(
      const TriangleMesh& mesh,
      const MeshDiagnosticsReport& diagnostics,
      const MeshReadinessReport& readiness);

  static MeshFeatureSummary fromSTL(
      const std::string& stl_path,
      MeshDiagnosticsReport* diagnostics_out = nullptr,
      MeshReadinessReport* readiness_out = nullptr);
};

}  // namespace adasdf

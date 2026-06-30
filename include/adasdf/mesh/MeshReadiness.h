#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/mesh/MeshDiagnostics.h"

namespace adasdf {

enum class MeshIssueSeverity {
  Info,
  Warning,
  Critical
};

enum class MeshReadinessLevel {
  Ready,
  UsableWithWarnings,
  Poor,
  NotRecommended
};

struct MeshIssue {
  MeshIssueSeverity severity = MeshIssueSeverity::Info;
  std::string code;
  std::string message;
  std::string suggestion;
};

struct MeshReadinessOptions {
  bool require_watertight = true;
  bool allow_multiple_components = true;
  bool allow_duplicate_triangles = false;
  bool allow_degenerate_triangles = false;
  bool allow_non_manifold_edges = false;

  std::size_t max_boundary_edges_warning = 0;
  std::size_t max_duplicate_triangles_warning = 0;
  std::size_t max_degenerate_triangles_warning = 0;

  double very_small_diagonal_threshold = 1e-6;
  double very_large_diagonal_threshold = 1e6;
};

struct MeshReadinessReport {
  MeshReadinessLevel level = MeshReadinessLevel::NotRecommended;
  int score = 0;

  std::vector<MeshIssue> issues;
  std::vector<std::string> recommended_steps;

  bool recommended_for_sdf_build = false;
  bool recommended_for_contact_query = false;

  std::string summary;
};

const char* toString(MeshIssueSeverity severity);
const char* toString(MeshReadinessLevel level);

class MeshReadiness {
 public:
  static MeshReadinessReport evaluate(
      const MeshDiagnosticsReport& diagnostics,
      const MeshReadinessOptions& options = MeshReadinessOptions{});
};

}  // namespace adasdf

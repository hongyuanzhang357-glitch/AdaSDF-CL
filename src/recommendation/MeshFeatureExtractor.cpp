#include "adasdf/recommendation/MeshFeatureExtractor.h"

#include <algorithm>
#include <cmath>

#include "adasdf/mesh/STLReader.h"

namespace adasdf {
namespace {

double safeDelta(double a, double b) {
  const double value = b - a;
  return std::isfinite(value) && value > 0.0 ? value : 0.0;
}

void appendUnique(
    std::vector<std::string>& values,
    const std::string& value) {
  if (value.empty()) {
    return;
  }
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

}  // namespace

MeshFeatureSummary MeshFeatureExtractor::fromMesh(
    const TriangleMesh& mesh,
    const MeshDiagnosticsReport& diagnostics,
    const MeshReadinessReport& readiness) {
  MeshFeatureSummary features;
  features.vertex_count = diagnostics.vertex_count;
  features.triangle_count = diagnostics.triangle_count;
  if (features.vertex_count == 0) {
    features.vertex_count = mesh.vertexCount();
  }
  if (features.triangle_count == 0) {
    features.triangle_count = mesh.triangleCount();
  }

  const double dx = safeDelta(diagnostics.aabb.min.x, diagnostics.aabb.max.x);
  const double dy = safeDelta(diagnostics.aabb.min.y, diagnostics.aabb.max.y);
  const double dz = safeDelta(diagnostics.aabb.min.z, diagnostics.aabb.max.z);
  features.aabb_volume = dx * dy * dz;
  features.aabb_diagonal =
      std::isfinite(diagnostics.diagonal_length)
          ? std::max(0.0, diagnostics.diagonal_length)
          : 0.0;
  if (features.aabb_diagonal == 0.0 && !mesh.empty()) {
    features.aabb_diagonal = mesh.diagonalLength();
  }
  features.has_degenerate_aabb =
      !(features.aabb_diagonal > 0.0) ||
      !std::isfinite(features.aabb_diagonal) ||
      !(features.aabb_volume > 0.0) ||
      !std::isfinite(features.aabb_volume);

  features.watertight = diagnostics.watertight;
  features.likely_oriented = diagnostics.likely_oriented;
  features.has_nan_or_inf = diagnostics.has_nan_or_inf;
  features.boundary_edge_count = diagnostics.boundary_edge_count;
  features.non_manifold_edge_count = diagnostics.non_manifold_edge_count;
  features.degenerate_triangle_count = diagnostics.degenerate_triangle_count;
  features.duplicate_triangle_count = diagnostics.duplicate_triangle_count;
  features.connected_component_count = diagnostics.connected_component_count;
  features.readiness_score = readiness.score;
  features.readiness_level = toString(readiness.level);
  features.valid =
      diagnostics.valid_mesh && features.triangle_count > 0 &&
      !features.has_nan_or_inf && !features.has_degenerate_aabb;

  for (const std::string& warning : diagnostics.warnings) {
    appendUnique(features.warnings, warning);
  }
  for (const std::string& error : diagnostics.errors) {
    appendUnique(features.warnings, error);
  }
  for (const MeshIssue& issue : readiness.issues) {
    if (issue.severity != MeshIssueSeverity::Info) {
      appendUnique(features.warnings, issue.message);
    }
  }
  if (features.has_degenerate_aabb) {
    appendUnique(
        features.warnings,
        "mesh AABB is degenerate; recommendation confidence is low");
  }
  if (!features.valid) {
    appendUnique(
        features.warnings,
        "mesh features are not valid enough for a reliable recommendation");
  }
  return features;
}

MeshFeatureSummary MeshFeatureExtractor::fromSTL(
    const std::string& stl_path,
    MeshDiagnosticsReport* diagnostics_out,
    MeshReadinessReport* readiness_out) {
  const STLReadResult read = STLReader::read(stl_path);
  if (!read.success) {
    MeshFeatureSummary features;
    features.valid = false;
    features.warnings.push_back("failed to read STL: " + read.error_message);
    if (diagnostics_out) {
      *diagnostics_out = MeshDiagnosticsReport{};
      diagnostics_out->errors.push_back(features.warnings.back());
    }
    if (readiness_out) {
      *readiness_out = MeshReadinessReport{};
      readiness_out->summary = "STL could not be read.";
    }
    return features;
  }

  MeshDiagnosticsReport diagnostics = MeshDiagnostics::analyze(read.mesh);
  MeshReadinessOptions readiness_options;
  readiness_options.require_watertight = true;
  MeshReadinessReport readiness =
      MeshReadiness::evaluate(diagnostics, readiness_options);
  if (diagnostics_out) {
    *diagnostics_out = diagnostics;
  }
  if (readiness_out) {
    *readiness_out = readiness;
  }
  return fromMesh(read.mesh, diagnostics, readiness);
}

}  // namespace adasdf

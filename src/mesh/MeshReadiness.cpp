#include "adasdf/mesh/MeshReadiness.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace adasdf {

namespace {

void addIssue(
    MeshReadinessReport& report,
    MeshIssueSeverity severity,
    const std::string& code,
    const std::string& message,
    const std::string& suggestion) {
  report.issues.push_back(MeshIssue{severity, code, message, suggestion});
  if (severity != MeshIssueSeverity::Info) {
    const bool exists =
        std::find(
            report.recommended_steps.begin(),
            report.recommended_steps.end(),
            suggestion) != report.recommended_steps.end();
    if (!exists) {
      report.recommended_steps.push_back(suggestion);
    }
  }
}

bool hasCriticalIssue(const MeshReadinessReport& report) {
  return std::any_of(
      report.issues.begin(),
      report.issues.end(),
      [](const MeshIssue& issue) {
        return issue.severity == MeshIssueSeverity::Critical;
      });
}

int clampScore(int score) {
  return std::max(0, std::min(100, score));
}

std::string countMessage(
    const char* prefix,
    std::size_t count,
    const char* suffix) {
  std::ostringstream out;
  out << prefix << count << suffix;
  return out.str();
}

}  // namespace

const char* toString(MeshIssueSeverity severity) {
  switch (severity) {
    case MeshIssueSeverity::Info:
      return "Info";
    case MeshIssueSeverity::Warning:
      return "Warning";
    case MeshIssueSeverity::Critical:
      return "Critical";
  }
  return "Info";
}

const char* toString(MeshReadinessLevel level) {
  switch (level) {
    case MeshReadinessLevel::Ready:
      return "Ready";
    case MeshReadinessLevel::UsableWithWarnings:
      return "UsableWithWarnings";
    case MeshReadinessLevel::Poor:
      return "Poor";
    case MeshReadinessLevel::NotRecommended:
      return "NotRecommended";
  }
  return "NotRecommended";
}

MeshReadinessReport MeshReadiness::evaluate(
    const MeshDiagnosticsReport& diagnostics,
    const MeshReadinessOptions& options) {
  MeshReadinessReport report;
  int score = 100;

  addIssue(
      report,
      MeshIssueSeverity::Info,
      "AABB_SUMMARY",
      "Mesh AABB diagonal is " + std::to_string(diagnostics.diagonal_length) +
          ".",
      "Confirm the STL units and scale before adaptive SDF construction.");

  addIssue(
      report,
      MeshIssueSeverity::Info,
      "COMPONENT_COUNT",
      "Mesh has " + std::to_string(diagnostics.connected_component_count) +
          " connected component(s).",
      "Confirm whether separate components are expected for this model.");

  if (diagnostics.triangle_count == 0) {
    score -= 100;
    addIssue(
        report,
        MeshIssueSeverity::Critical,
        "EMPTY_MESH",
        "Mesh has no triangles.",
        "Export a non-empty closed STL before adaptive SDF construction.");
  }

  if (diagnostics.has_nan_or_inf) {
    score -= 100;
    addIssue(
        report,
        MeshIssueSeverity::Critical,
        "NAN_OR_INF",
        "Mesh contains NaN, Inf, or invalid geometry.",
        "Re-export or repair the STL so all vertices and faces are finite.");
  }

  if (diagnostics.diagonal_length == 0.0 ||
      !std::isfinite(diagnostics.diagonal_length)) {
    score -= 80;
    addIssue(
        report,
        MeshIssueSeverity::Critical,
        "ZERO_DIAGONAL",
        "Mesh AABB diagonal is zero or invalid.",
        "Check that the STL contains a real three-dimensional surface.");
  }

  if (diagnostics.non_manifold_edge_count > 0) {
    if (options.allow_non_manifold_edges) {
      score -= 20;
      addIssue(
          report,
          MeshIssueSeverity::Warning,
          "NON_MANIFOLD_EDGES",
          countMessage(
              "Mesh has ",
              diagnostics.non_manifold_edge_count,
              " non-manifold edge(s)."),
          "Repair non-manifold topology before relying on SDF contacts.");
    } else {
      score -= 40;
      addIssue(
          report,
          MeshIssueSeverity::Critical,
          "NON_MANIFOLD_EDGES",
          countMessage(
              "Mesh has ",
              diagnostics.non_manifold_edge_count,
              " non-manifold edge(s)."),
          "Remove non-manifold edges before adaptive SDF construction.");
    }
  }

  if (diagnostics.boundary_edge_count > 0) {
    const bool warning_only =
        !options.require_watertight ||
        diagnostics.boundary_edge_count <= options.max_boundary_edges_warning;
    if (warning_only) {
      score -= 15;
      addIssue(
          report,
          MeshIssueSeverity::Warning,
          "BOUNDARY_EDGES",
          countMessage(
              "Mesh has ",
              diagnostics.boundary_edge_count,
              " boundary edge(s) and is open."),
          "Fill holes if a closed solid is required for the downstream SDF.");
    } else {
      score -= 35;
      addIssue(
          report,
          MeshIssueSeverity::Critical,
          "BOUNDARY_EDGES",
          countMessage(
              "Mesh has ",
              diagnostics.boundary_edge_count,
              " boundary edge(s) and is not watertight."),
          "Fill holes or repair the STL before adaptive SDF construction.");
    }
  }

  if (diagnostics.degenerate_triangle_count >
      options.max_degenerate_triangles_warning) {
    if (options.allow_degenerate_triangles) {
      score -= 15;
      addIssue(
          report,
          MeshIssueSeverity::Warning,
          "DEGENERATE_TRIANGLES",
          countMessage(
              "Mesh has ",
              diagnostics.degenerate_triangle_count,
              " degenerate triangle(s)."),
          "Remove zero-area or repeated-vertex triangles before final use.");
    } else {
      score -= 35;
      addIssue(
          report,
          MeshIssueSeverity::Critical,
          "DEGENERATE_TRIANGLES",
          countMessage(
              "Mesh has ",
              diagnostics.degenerate_triangle_count,
              " degenerate triangle(s)."),
          "Remove degenerate triangles before adaptive SDF construction.");
    }
  }

  if (diagnostics.duplicate_triangle_count >
      options.max_duplicate_triangles_warning) {
    score -= options.allow_duplicate_triangles ? 5 : 10;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "DUPLICATE_TRIANGLES",
        countMessage(
            "Mesh has ",
            diagnostics.duplicate_triangle_count,
            " duplicate triangle candidate(s)."),
        "Remove duplicate triangles to avoid redundant surfaces and unstable "
        "sampling.");
  }

  if (!options.allow_multiple_components &&
      diagnostics.connected_component_count > 1) {
    score -= 20;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "MULTIPLE_COMPONENTS",
        countMessage(
            "Mesh has ",
            diagnostics.connected_component_count,
            " connected component(s)."),
        "Split unrelated parts or confirm that multi-component SDF input is "
        "intended.");
  } else if (diagnostics.connected_component_count > 1) {
    score -= 10;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "MULTIPLE_COMPONENTS",
        countMessage(
            "Mesh has ",
            diagnostics.connected_component_count,
            " connected component(s)."),
        "Confirm that each component belongs in the same SDF asset.");
  }

  if (diagnostics.isolated_vertex_count > 0) {
    score -= 5;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "ISOLATED_VERTICES",
        countMessage(
            "Mesh has ",
            diagnostics.isolated_vertex_count,
            " isolated vertex/vertices."),
        "Remove isolated vertices during mesh cleanup.");
  }

  if (diagnostics.diagonal_length > 0.0 &&
      diagnostics.diagonal_length < options.very_small_diagonal_threshold) {
    score -= 5;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "VERY_SMALL_SCALE",
        "Mesh scale is very small.",
        "Confirm whether the STL units should be converted before SDF build.");
  }

  if (diagnostics.diagonal_length > options.very_large_diagonal_threshold) {
    score -= 5;
    addIssue(
        report,
        MeshIssueSeverity::Warning,
        "VERY_LARGE_SCALE",
        "Mesh scale is very large.",
        "Confirm whether the STL units should be converted before SDF build.");
  }

  if (report.recommended_steps.empty()) {
    report.recommended_steps.push_back(
        "Proceed to SDF build preflight with the selected build settings.");
  } else {
    report.recommended_steps.push_back("Rerun adasdf_mesh_check after cleanup.");
  }

  report.score = clampScore(score);
  if (report.score == 100 && !hasCriticalIssue(report)) {
    report.level = MeshReadinessLevel::Ready;
  } else if (report.score >= 80 && !hasCriticalIssue(report)) {
    report.level = MeshReadinessLevel::UsableWithWarnings;
  } else if (report.score >= 50) {
    report.level = MeshReadinessLevel::Poor;
  } else {
    report.level = MeshReadinessLevel::NotRecommended;
  }

  report.recommended_for_sdf_build =
      (report.level == MeshReadinessLevel::Ready ||
       report.level == MeshReadinessLevel::UsableWithWarnings) &&
      !hasCriticalIssue(report);
  report.recommended_for_contact_query =
      diagnostics.triangle_count > 0 && !diagnostics.has_nan_or_inf &&
      diagnostics.non_manifold_edge_count == 0 &&
      report.level != MeshReadinessLevel::NotRecommended;

  if (report.level == MeshReadinessLevel::Ready) {
    report.summary =
        "Mesh is ready for adaptive SDF build preflight. This is not a full "
        "geometric certification.";
  } else if (report.level == MeshReadinessLevel::UsableWithWarnings) {
    report.summary =
        "Mesh is usable with warnings; review preprocessing suggestions before "
        "building an adaptive SDF.";
  } else if (report.level == MeshReadinessLevel::Poor) {
    report.summary =
        "Mesh is poor for adaptive SDF construction and should be preprocessed.";
  } else {
    report.summary =
        "Mesh is not recommended for adaptive SDF construction in its current "
        "state.";
  }

  return report;
}

}  // namespace adasdf

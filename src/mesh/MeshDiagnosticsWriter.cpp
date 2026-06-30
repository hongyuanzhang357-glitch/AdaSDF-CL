#include "adasdf/mesh/MeshDiagnosticsWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {

namespace {

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

std::string escapeJson(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    if (ch == '\n') {
      out += "\\n";
    } else {
      out.push_back(ch);
    }
  }
  return out;
}

void writeStringArray(
    std::ostringstream& out,
    const char* key,
    const std::vector<std::string>& values,
    bool comma) {
  out << "  \"" << key << "\": [";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << "\"" << escapeJson(values[i]) << "\"";
  }
  out << "]";
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeIssueArray(
    std::ostringstream& out,
    const char* key,
    const std::vector<MeshIssue>& issues,
    bool comma) {
  out << "  \"" << key << "\": [\n";
  for (std::size_t i = 0; i < issues.size(); ++i) {
    const MeshIssue& issue = issues[i];
    out << "    {\n";
    out << "      \"severity\": \"" << toString(issue.severity) << "\",\n";
    out << "      \"code\": \"" << escapeJson(issue.code) << "\",\n";
    out << "      \"message\": \"" << escapeJson(issue.message) << "\",\n";
    out << "      \"suggestion\": \"" << escapeJson(issue.suggestion) << "\"\n";
    out << "    }";
    if (i + 1 < issues.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]";
  if (comma) {
    out << ",";
  }
  out << "\n";
}

void writeIssueGroup(
    std::ostringstream& out,
    const MeshReadinessReport& readiness,
    MeshIssueSeverity severity) {
  bool wrote = false;
  for (const MeshIssue& issue : readiness.issues) {
    if (issue.severity == severity) {
      out << "- " << issue.code << ": " << issue.message << "\n";
      out << "  Suggestion: " << issue.suggestion << "\n";
      wrote = true;
    }
  }
  if (!wrote) {
    out << "- none\n";
  }
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

}  // namespace

std::string MeshDiagnosticsWriter::toMarkdown(
    const MeshDiagnosticsReport& report) {
  std::ostringstream out;
  out << "# Mesh Diagnostics Report\n\n";
  out << "## Summary\n\n";
  out << "- Valid mesh: " << yesNo(report.valid_mesh) << "\n";
  out << "- Watertight: " << yesNo(report.watertight) << "\n";
  out << "- Likely oriented: " << yesNo(report.likely_oriented) << "\n";
  out << "- Recommendation: " << report.recommendation << "\n\n";

  out << "## AABB / Scale\n\n";
  out << "- Min: " << report.aabb.min.x << " " << report.aabb.min.y << " "
      << report.aabb.min.z << "\n";
  out << "- Max: " << report.aabb.max.x << " " << report.aabb.max.y << " "
      << report.aabb.max.z << "\n";
  out << "- Diagonal: " << report.diagonal_length << "\n";
  out << "- Very small scale warning: "
      << yesNo(report.has_small_scale_warning) << "\n";
  out << "- Very large scale warning: "
      << yesNo(report.has_extreme_scale_warning) << "\n\n";

  out << "## Counts\n\n";
  out << "- Vertices: " << report.vertex_count << "\n";
  out << "- Triangles: " << report.triangle_count << "\n";
  out << "- Raw triangles: " << report.raw_triangle_count << "\n";
  out << "- Connected components: " << report.connected_component_count << "\n";
  out << "- Isolated vertices: " << report.isolated_vertex_count << "\n\n";

  out << "## Watertight Status\n\n";
  out << "- Boundary edges: " << report.boundary_edge_count << "\n";
  out << "- Non-manifold edges: " << report.non_manifold_edge_count << "\n\n";

  out << "## Problems\n\n";
  out << "- Degenerate triangles: " << report.degenerate_triangle_count << "\n";
  out << "- Duplicate triangles: " << report.duplicate_triangle_count << "\n";
  out << "- Zero-area faces: " << report.zero_area_face_count << "\n";
  out << "- NaN / Inf: " << yesNo(report.has_nan_or_inf) << "\n\n";

  out << "## Sample Bad Elements\n\n";
  out << "- Degenerate faces:";
  for (int id : report.sample_degenerate_faces) {
    out << " " << id;
  }
  out << "\n- Duplicate faces:";
  for (int id : report.sample_duplicate_faces) {
    out << " " << id;
  }
  out << "\n- Boundary edges:";
  for (const auto& edge : report.sample_boundary_edges) {
    out << " (" << edge.first << "," << edge.second << ")";
  }
  out << "\n- Non-manifold edges:";
  for (const auto& edge : report.sample_non_manifold_edges) {
    out << " (" << edge.first << "," << edge.second << ")";
  }
  out << "\n\n";

  out << "## Warnings\n\n";
  if (report.warnings.empty()) {
    out << "- none\n";
  } else {
    for (const std::string& warning : report.warnings) {
      out << "- " << warning << "\n";
    }
  }
  out << "\n## Errors\n\n";
  if (report.errors.empty()) {
    out << "- none\n";
  } else {
    for (const std::string& error : report.errors) {
      out << "- " << error << "\n";
    }
  }
  out << "\n## Recommendation\n\n" << report.recommendation << "\n";
  return out.str();
}

std::string MeshDiagnosticsWriter::toJson(
    const MeshDiagnosticsReport& report) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"valid_mesh\": " << (report.valid_mesh ? "true" : "false")
      << ",\n";
  out << "  \"watertight\": " << (report.watertight ? "true" : "false")
      << ",\n";
  out << "  \"likely_oriented\": "
      << (report.likely_oriented ? "true" : "false") << ",\n";
  out << "  \"vertex_count\": " << report.vertex_count << ",\n";
  out << "  \"triangle_count\": " << report.triangle_count << ",\n";
  out << "  \"raw_triangle_count\": " << report.raw_triangle_count << ",\n";
  out << "  \"aabb\": {\n";
  out << "    \"min\": [" << report.aabb.min.x << ", " << report.aabb.min.y
      << ", " << report.aabb.min.z << "],\n";
  out << "    \"max\": [" << report.aabb.max.x << ", " << report.aabb.max.y
      << ", " << report.aabb.max.z << "]\n";
  out << "  },\n";
  out << "  \"diagonal_length\": " << report.diagonal_length << ",\n";
  out << "  \"degenerate_triangle_count\": "
      << report.degenerate_triangle_count << ",\n";
  out << "  \"duplicate_triangle_count\": "
      << report.duplicate_triangle_count << ",\n";
  out << "  \"boundary_edge_count\": " << report.boundary_edge_count << ",\n";
  out << "  \"non_manifold_edge_count\": "
      << report.non_manifold_edge_count << ",\n";
  out << "  \"connected_component_count\": "
      << report.connected_component_count << ",\n";
  out << "  \"isolated_vertex_count\": " << report.isolated_vertex_count
      << ",\n";
  out << "  \"zero_area_face_count\": " << report.zero_area_face_count << ",\n";
  out << "  \"has_nan_or_inf\": "
      << (report.has_nan_or_inf ? "true" : "false") << ",\n";
  out << "  \"has_extreme_scale_warning\": "
      << (report.has_extreme_scale_warning ? "true" : "false") << ",\n";
  out << "  \"has_small_scale_warning\": "
      << (report.has_small_scale_warning ? "true" : "false") << ",\n";
  writeStringArray(out, "warnings", report.warnings, true);
  writeStringArray(out, "errors", report.errors, true);
  out << "  \"recommendation\": \"" << escapeJson(report.recommendation)
      << "\"\n";
  out << "}\n";
  return out.str();
}

std::string MeshDiagnosticsWriter::readinessToMarkdown(
    const MeshReadinessReport& readiness) {
  std::ostringstream out;
  out << "# SDF Build Readiness Report\n\n";
  out << "## SDF Build Readiness\n\n";
  out << "- Level: " << toString(readiness.level) << "\n";
  out << "- Score: " << readiness.score << " / 100\n";
  out << "- Recommended for SDF build: "
      << yesNo(readiness.recommended_for_sdf_build) << "\n";
  out << "- Recommended for contact query: "
      << yesNo(readiness.recommended_for_contact_query) << "\n";
  out << "- Summary: " << readiness.summary << "\n\n";

  out << "## Critical Issues\n\n";
  writeIssueGroup(out, readiness, MeshIssueSeverity::Critical);
  out << "\n## Warnings\n\n";
  writeIssueGroup(out, readiness, MeshIssueSeverity::Warning);
  out << "\n## Info\n\n";
  writeIssueGroup(out, readiness, MeshIssueSeverity::Info);

  out << "\n## Recommended Preprocessing Steps\n\n";
  for (std::size_t i = 0; i < readiness.recommended_steps.size(); ++i) {
    out << (i + 1) << ". " << readiness.recommended_steps[i] << "\n";
  }
  out << "\nReadiness is a preflight heuristic, not an industrial "
         "certification or automatic mesh repair.\n";
  return out.str();
}

std::string MeshDiagnosticsWriter::readinessToJson(
    const MeshReadinessReport& readiness) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"level\": \"" << toString(readiness.level) << "\",\n";
  out << "  \"score\": " << readiness.score << ",\n";
  out << "  \"recommended_for_sdf_build\": "
      << (readiness.recommended_for_sdf_build ? "true" : "false") << ",\n";
  out << "  \"recommended_for_contact_query\": "
      << (readiness.recommended_for_contact_query ? "true" : "false")
      << ",\n";
  out << "  \"summary\": \"" << escapeJson(readiness.summary) << "\",\n";
  writeIssueArray(out, "issues", readiness.issues, true);
  writeStringArray(out, "recommended_steps", readiness.recommended_steps, false);
  out << "}\n";
  return out.str();
}

std::string MeshDiagnosticsWriter::combinedMarkdown(
    const MeshDiagnosticsReport& diagnostics,
    const MeshReadinessReport& readiness) {
  return toMarkdown(diagnostics) + "\n\n" + readinessToMarkdown(readiness);
}

std::string MeshDiagnosticsWriter::combinedJson(
    const MeshDiagnosticsReport& diagnostics,
    const MeshReadinessReport& readiness) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"diagnostics\": ";
  const std::string diagnostics_json = toJson(diagnostics);
  for (std::size_t i = 0; i < diagnostics_json.size(); ++i) {
    const char ch = diagnostics_json[i];
    out << ch;
    if (ch == '\n' && i + 1 < diagnostics_json.size()) {
      out << "  ";
    }
  }
  out << ",\n";
  out << "  \"readiness\": ";
  const std::string readiness_json = readinessToJson(readiness);
  for (std::size_t i = 0; i < readiness_json.size(); ++i) {
    const char ch = readiness_json[i];
    out << ch;
    if (ch == '\n' && i + 1 < readiness_json.size()) {
      out << "  ";
    }
  }
  out << "}\n";
  return out.str();
}

void MeshDiagnosticsWriter::writeMarkdown(
    const std::string& path_string,
    const MeshDiagnosticsReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toMarkdown(report);
}

void MeshDiagnosticsWriter::writeJson(
    const std::string& path_string,
    const MeshDiagnosticsReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toJson(report);
}

}  // namespace adasdf

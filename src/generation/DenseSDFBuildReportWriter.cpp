#include "adasdf/generation/DenseSDFBuildReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "adasdf/acceleration/BuildAccelerationReport.h"

namespace adasdf {
namespace {

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string escaped(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

}  // namespace

std::string DenseSDFBuildReportWriter::toMarkdown(
    const DenseSDFBuildReport& report) {
  std::ostringstream out;
  out << "# DenseSDF Build Report\n\n";
  out << "## Summary\n\n";
  out << "- Success: " << yesNo(report.success) << "\n";
  if (!report.error_message.empty()) {
    out << "- Error: " << report.error_message << "\n";
  }
  out << "- Signed distance: " << yesNo(report.signed_distance) << "\n";
  out << "- Used cleanup: " << yesNo(report.used_cleanup) << "\n";
  out << "- Watertight: " << yesNo(report.watertight) << "\n\n";

  out << "## Input Mesh Diagnostics\n\n";
  out << "- Vertices: " << report.vertex_count << "\n";
  out << "- Triangles: " << report.triangle_count << "\n";
  out << "- Boundary edges: " << report.diagnostics.boundary_edge_count << "\n";
  out << "- Non-manifold edges: "
      << report.diagnostics.non_manifold_edge_count << "\n";
  out << "- Degenerate triangles: "
      << report.diagnostics.degenerate_triangle_count << "\n";
  out << "- Duplicate triangles: "
      << report.diagnostics.duplicate_triangle_count << "\n\n";

  out << "## Readiness\n\n";
  out << "- Level: " << toString(report.readiness.level) << "\n";
  out << "- Score: " << report.readiness.score << " / 100\n";
  out << "- Summary: " << report.readiness.summary << "\n\n";

  out << "## Dense Grid\n\n";
  out << "- Resolution: " << report.nx << " x " << report.ny << " x "
      << report.nz << "\n";
  out << "- Padding: " << report.padding << "\n";
  out << "- Memory bytes: " << report.memory_bytes << "\n";
  out << "- Build time ms: " << report.build_time_ms << "\n\n";

  out << adasdf::toMarkdown(report.acceleration_stats) << "\n\n";

  out << "## Warnings\n\n";
  if (report.warnings.empty()) {
    out << "- none\n\n";
  } else {
    for (const std::string& warning : report.warnings) {
      out << "- " << warning << "\n";
    }
    out << "\n";
  }

  out << "## Limitations\n\n";
  out << "- Brute-force triangle distance remains the default reference path.\n";
  out << "- BVH acceleration is an optional CPU path for builder sampling.\n";
  out << "- Sign classification uses alpha-level ray casting.\n";
  out << "- This is a uniform dense SDF builder, not the adaptive "
         "octree/block/low-rank builder.\n";
  return out.str();
}

std::string DenseSDFBuildReportWriter::toJson(
    const DenseSDFBuildReport& report) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << (report.success ? "true" : "false") << ",\n";
  out << "  \"error_message\": \"" << escaped(report.error_message) << "\",\n";
  out << "  \"signed_distance\": "
      << (report.signed_distance ? "true" : "false") << ",\n";
  out << "  \"used_cleanup\": " << (report.used_cleanup ? "true" : "false")
      << ",\n";
  out << "  \"watertight\": " << (report.watertight ? "true" : "false")
      << ",\n";
  out << "  \"resolution\": [" << report.nx << ", " << report.ny << ", "
      << report.nz << "],\n";
  out << "  \"padding\": " << report.padding << ",\n";
  out << "  \"build_time_ms\": " << report.build_time_ms << ",\n";
  out << "  \"acceleration\": " << adasdf::toJson(report.acceleration_stats)
      << ",\n";
  out << "  \"triangle_count\": " << report.triangle_count << ",\n";
  out << "  \"vertex_count\": " << report.vertex_count << ",\n";
  out << "  \"memory_bytes\": " << report.memory_bytes << ",\n";
  out << "  \"readiness\": \"" << toString(report.readiness.level) << "\",\n";
  out << "  \"score\": " << report.readiness.score << ",\n";
  out << "  \"warnings\": [";
  for (std::size_t i = 0; i < report.warnings.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << "\"" << escaped(report.warnings[i]) << "\"";
  }
  out << "]\n";
  out << "}\n";
  return out.str();
}

void DenseSDFBuildReportWriter::writeMarkdown(
    const std::string& path_string,
    const DenseSDFBuildReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toMarkdown(report);
}

void DenseSDFBuildReportWriter::writeJson(
    const std::string& path_string,
    const DenseSDFBuildReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toJson(report);
}

}  // namespace adasdf

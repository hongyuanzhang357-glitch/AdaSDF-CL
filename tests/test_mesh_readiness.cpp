#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <string>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

std::filesystem::path fixture(const std::string& name) {
  return std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / name;
}

adasdf::MeshDiagnosticsReport analyze(const std::string& name) {
  const auto read = adasdf::STLReader::read(fixture(name).string());
  if (!read.success) {
    throw std::runtime_error("failed to read fixture: " + name + ": " +
                             read.error_message);
  }
  auto report = adasdf::MeshDiagnostics::analyze(read.mesh);
  report.raw_triangle_count = read.raw_triangle_count;
  return report;
}

bool hasIssue(
    const adasdf::MeshReadinessReport& report,
    const std::string& code,
    adasdf::MeshIssueSeverity severity) {
  for (const adasdf::MeshIssue& issue : report.issues) {
    if (issue.code == code && issue.severity == severity &&
        !issue.message.empty() && !issue.suggestion.empty()) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  try {
    const auto closed =
        adasdf::MeshReadiness::evaluate(analyze("closed_cube_ascii.stl"));
    if (closed.level != adasdf::MeshReadinessLevel::Ready ||
        closed.score != 100 || !closed.recommended_for_sdf_build) {
      std::cerr << "closed cube should be ready\n";
      return 1;
    }

    const auto open_default = adasdf::MeshReadiness::evaluate(
        analyze("open_cube_missing_face_ascii.stl"));
    if (open_default.level != adasdf::MeshReadinessLevel::Poor ||
        open_default.recommended_for_sdf_build ||
        !hasIssue(
            open_default,
            "BOUNDARY_EDGES",
            adasdf::MeshIssueSeverity::Critical)) {
      std::cerr << "open cube should have critical boundary readiness issue\n";
      return 1;
    }

    const auto non_manifold = adasdf::MeshReadiness::evaluate(
        analyze("non_manifold_edge_ascii.stl"));
    if (!hasIssue(
            non_manifold,
            "NON_MANIFOLD_EDGES",
            adasdf::MeshIssueSeverity::Critical)) {
      std::cerr << "non-manifold edge should be critical\n";
      return 1;
    }

    const auto degenerate =
        adasdf::MeshReadiness::evaluate(analyze("degenerate_triangle_ascii.stl"));
    if (!hasIssue(
            degenerate,
            "DEGENERATE_TRIANGLES",
            adasdf::MeshIssueSeverity::Critical)) {
      std::cerr << "degenerate triangle should be critical by default\n";
      return 1;
    }

    const auto duplicate =
        adasdf::MeshReadiness::evaluate(analyze("duplicate_triangle_ascii.stl"));
    if (!hasIssue(
            duplicate,
            "DUPLICATE_TRIANGLES",
            adasdf::MeshIssueSeverity::Warning)) {
      std::cerr << "duplicate triangle should be a warning\n";
      return 1;
    }

    adasdf::MeshReadinessOptions lenient;
    lenient.require_watertight = false;
    lenient.allow_duplicate_triangles = true;
    lenient.allow_degenerate_triangles = true;
    const auto open_lenient = adasdf::MeshReadiness::evaluate(
        analyze("open_cube_missing_face_ascii.stl"),
        lenient);
    if (open_lenient.score <= open_default.score ||
        open_lenient.level != adasdf::MeshReadinessLevel::UsableWithWarnings ||
        !hasIssue(
            open_lenient,
            "BOUNDARY_EDGES",
            adasdf::MeshIssueSeverity::Warning)) {
      std::cerr << "lenient open mesh readiness should downgrade boundary issue\n";
      return 1;
    }

    if (adasdf::toString(adasdf::MeshReadinessLevel::Ready) !=
            std::string("Ready") ||
        adasdf::toString(adasdf::MeshIssueSeverity::Critical) !=
            std::string("Critical")) {
      std::cerr << "readiness string conversions failed\n";
      return 1;
    }

    std::cout << "mesh readiness passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_readiness failed: " << exc.what() << "\n";
    return 1;
  }
}

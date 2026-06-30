#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    const auto stl = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
                     "closed_cube_ascii.stl";
    const auto read = adasdf::STLReader::read(stl.string());
    if (!read.success) {
      std::cerr << "failed to read writer fixture\n";
      return 1;
    }
    auto report = adasdf::MeshDiagnostics::analyze(read.mesh);
    report.raw_triangle_count = read.raw_triangle_count;
    const auto readiness = adasdf::MeshReadiness::evaluate(report);

    const std::string markdown =
        adasdf::MeshDiagnosticsWriter::toMarkdown(report);
    const std::string json = adasdf::MeshDiagnosticsWriter::toJson(report);
    const std::string readiness_markdown =
        adasdf::MeshDiagnosticsWriter::readinessToMarkdown(readiness);
    const std::string combined_json =
        adasdf::MeshDiagnosticsWriter::combinedJson(report, readiness);
    const auto cleanup = adasdf::MeshCleanup::clean(read.mesh);
    const std::string cleanup_markdown =
        adasdf::MeshDiagnosticsWriter::cleanupToMarkdown(cleanup.stats);
    if (!contains(markdown, "Watertight") ||
        !contains(json, "\"triangle_count\"") ||
        !contains(readiness_markdown, "SDF Build Readiness") ||
        !contains(combined_json, "\"readiness\"") ||
        !contains(cleanup_markdown, "Cleanup Operations")) {
      std::cerr << "diagnostics writer output missing required fields\n";
      return 1;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const std::filesystem::path md_path = temp / "mesh_report.md";
    const std::filesystem::path json_path = temp / "mesh_report.json";
    adasdf::MeshDiagnosticsWriter::writeMarkdown(md_path.string(), report);
    adasdf::MeshDiagnosticsWriter::writeJson(json_path.string(), report);
    if (!contains(readFile(md_path), "Recommendation") ||
        !contains(readFile(json_path), "\"watertight\"")) {
      std::cerr << "diagnostics writer files were not written correctly\n";
      return 1;
    }

    std::cout << "mesh diagnostics writer passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_diagnostics_writer failed: " << exc.what()
              << "\n";
    return 1;
  }
}

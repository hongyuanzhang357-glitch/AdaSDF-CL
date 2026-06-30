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

}  // namespace

int main() {
  try {
    const auto closed = analyze("closed_cube_ascii.stl");
    if (!closed.watertight || closed.boundary_edge_count != 0 ||
        closed.non_manifold_edge_count != 0 ||
        closed.connected_component_count != 1) {
      std::cerr << "closed cube diagnostics failed\n";
      return 1;
    }

    const auto open = analyze("open_cube_missing_face_ascii.stl");
    if (open.boundary_edge_count == 0 || open.watertight) {
      std::cerr << "open cube boundary edges were not detected\n";
      return 1;
    }

    const auto degenerate = analyze("degenerate_triangle_ascii.stl");
    if (degenerate.degenerate_triangle_count == 0) {
      std::cerr << "degenerate triangle was not detected\n";
      return 1;
    }

    const auto duplicate = analyze("duplicate_triangle_ascii.stl");
    if (duplicate.duplicate_triangle_count == 0) {
      std::cerr << "duplicate triangle was not detected\n";
      return 1;
    }

    const auto non_manifold = analyze("non_manifold_edge_ascii.stl");
    if (non_manifold.non_manifold_edge_count == 0) {
      std::cerr << "non-manifold edge was not detected\n";
      return 1;
    }

    const auto components = analyze("two_components_ascii.stl");
    if (components.connected_component_count != 2) {
      std::cerr << "two connected components were not detected\n";
      return 1;
    }

    std::cout << "mesh diagnostics passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_diagnostics failed: " << exc.what() << "\n";
    return 1;
  }
}

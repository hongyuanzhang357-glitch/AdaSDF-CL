#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
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
    const std::filesystem::path dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const std::vector<std::string> fixtures = {
        "closed_cube_ascii.stl",
        "open_cube_missing_face_ascii.stl",
        "degenerate_triangle_ascii.stl",
        "duplicate_triangle_ascii.stl",
        "non_manifold_edge_ascii.stl",
        "two_components_ascii.stl"};
    for (const std::string& name : fixtures) {
      const std::filesystem::path path = dir / name;
      if (!std::filesystem::exists(path)) {
        std::cerr << "missing mesh diagnostics fixture: " << name << "\n";
        return 1;
      }
      const std::string text = readFile(path);
      if (!contains(text, "project_generated")) {
        std::cerr << "fixture lacks project-generated marker: " << name
                  << "\n";
        return 1;
      }
      const auto read = adasdf::STLReader::read(path.string());
      if (!read.success || read.is_binary || read.mesh.triangleCount() == 0) {
        std::cerr << "fixture failed to read: " << name << "\n";
        return 1;
      }
    }

    std::cout << "mesh diagnostics fixtures passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_diagnostics_fixtures failed: " << exc.what()
              << "\n";
    return 1;
  }
}

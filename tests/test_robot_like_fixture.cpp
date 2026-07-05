#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

std::string readText(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  try {
    const std::filesystem::path path =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "robot_link_like_ascii.stl";
    if (!std::filesystem::exists(path)) {
      std::cerr << "robot-like fixture missing\n";
      return 1;
    }
    if (readText(path).find("project_generated") == std::string::npos) {
      std::cerr << "robot-like fixture lacks project-generated marker\n";
      return 1;
    }
    const auto read = adasdf::STLReader::read(path.string());
    if (!read.success || read.is_binary || read.mesh.triangleCount() == 0) {
      std::cerr << "robot-like fixture could not be read\n";
      return 1;
    }
    std::cout << "robot-like fixture passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_robot_like_fixture failed: " << exc.what() << "\n";
    return 1;
  }
}

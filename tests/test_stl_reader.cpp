#include <adasdf/adasdf.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::filesystem::path fixture(const std::string& name) {
  return std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / name;
}

void writeBinaryFixture(const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  char header[80] = {};
  const std::string label = "project-generated binary STL";
  label.copy(header, label.size());
  out.write(header, sizeof(header));
  const std::uint32_t triangles = 1;
  out.write(reinterpret_cast<const char*>(&triangles), sizeof(triangles));
  const float values[12] = {
      0.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f};
  out.write(reinterpret_cast<const char*>(values), sizeof(values));
  const std::uint16_t attribute = 0;
  out.write(reinterpret_cast<const char*>(&attribute), sizeof(attribute));
}

}  // namespace

int main() {
  try {
    const auto closed =
        adasdf::STLReader::read(fixture("closed_cube_ascii.stl").string());
    if (!closed.success || closed.is_binary || closed.mesh.vertexCount() == 0 ||
        closed.mesh.triangleCount() == 0) {
      std::cerr << "closed ASCII cube was not read correctly\n";
      return 1;
    }

    const auto missing =
        adasdf::STLReader::read(fixture("missing_file.stl").string());
    if (missing.success || missing.error_message.find("does not exist") ==
                               std::string::npos) {
      std::cerr << "missing STL did not return a clear failure\n";
      return 1;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    const std::filesystem::path binary_path = temp / "reader_binary.stl";
    writeBinaryFixture(binary_path);
    const auto binary = adasdf::STLReader::read(binary_path.string());
    if (!binary.success || !binary.is_binary ||
        binary.mesh.triangleCount() != 1 ||
        binary.mesh.vertexCount() != 3) {
      std::cerr << "binary STL was not read correctly\n";
      return 1;
    }

    std::cout << "STL reader passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_stl_reader failed: " << exc.what() << "\n";
    return 1;
  }
}

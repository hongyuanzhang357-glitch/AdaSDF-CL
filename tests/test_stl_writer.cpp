#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  try {
    const auto input =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const auto read = adasdf::STLReader::read(input.string());
    if (!read.success) {
      std::cerr << "failed to read fixture\n";
      return 1;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const auto output = temp / "stl_writer_roundtrip.stl";
    std::string error;
    if (!adasdf::STLWriter::write(output.string(), read.mesh, {}, &error)) {
      std::cerr << "STLWriter failed: " << error << "\n";
      return 1;
    }
    const auto reread = adasdf::STLReader::read(output.string());
    if (!reread.success || reread.mesh.triangleCount() != read.mesh.triangleCount()) {
      std::cerr << "written STL could not be reread\n";
      return 1;
    }

    adasdf::TriangleMesh empty;
    if (adasdf::STLWriter::write((temp / "empty.stl").string(), empty, {}, &error)) {
      std::cerr << "empty mesh write should fail\n";
      return 1;
    }

    std::cout << "stl writer passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_stl_writer failed: " << exc.what() << "\n";
    return 1;
  }
}

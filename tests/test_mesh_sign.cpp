#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const auto read = adasdf::STLReader::read(
        (fixture_dir / "closed_cube_ascii.stl").string());
    if (!read.success) {
      std::cerr << read.error_message << "\n";
      return 1;
    }
    const auto inside =
        adasdf::MeshSign::classifyPoint(read.mesh, {0.5, 0.5, 0.5});
    const auto outside =
        adasdf::MeshSign::classifyPoint(read.mesh, {1.5, 0.5, 0.5});
    const auto surface =
        adasdf::MeshSign::classifyPoint(read.mesh, {1.0, 0.5, 0.5});
    const auto inside_again =
        adasdf::MeshSign::classifyPoint(read.mesh, {0.5, 0.5, 0.5});
    if (inside != adasdf::MeshSignResult::Inside ||
        outside != adasdf::MeshSignResult::Outside ||
        inside_again != inside) {
      std::cerr << "inside/outside deterministic classification failed\n";
      return 1;
    }
    if (surface != adasdf::MeshSignResult::OnSurface &&
        surface != adasdf::MeshSignResult::Ambiguous) {
      std::cerr << "surface classification failed\n";
      return 1;
    }
    std::cout << "mesh sign passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_sign failed: " << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "wavy_sphere_ascii.stl";
  const auto read = adasdf::STLReader::read(fixture.string());
  if (!read.success || read.is_binary || read.mesh.triangleCount() < 16) {
    std::cerr << "wavy sphere fixture failed to read\n";
    return 1;
  }
  const auto diagnostics = adasdf::MeshDiagnostics::analyze(read.mesh);
  if (!diagnostics.valid_mesh || diagnostics.triangle_count < 16) {
    std::cerr << "wavy sphere diagnostics failed\n";
    return 1;
  }
  std::cout << "synthetic complex fixture passed\n";
  return 0;
}

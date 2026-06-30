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

adasdf::TriangleMesh readMesh(const std::string& name) {
  const auto read = adasdf::STLReader::read(fixture(name).string());
  if (!read.success) {
    throw std::runtime_error("failed to read fixture: " + read.error_message);
  }
  return read.mesh;
}

}  // namespace

int main() {
  try {
    const auto mesh = readMesh("duplicate_and_degenerate_ascii.stl");
    const auto before = adasdf::MeshDiagnostics::analyze(mesh);
    const auto cleanup = adasdf::MeshCleanup::clean(mesh);
    if (!cleanup.success) {
      std::cerr << "cleanup failed: " << cleanup.error_message << "\n";
      return 1;
    }
    const auto after = adasdf::MeshDiagnostics::analyze(cleanup.cleaned_mesh);
    if (cleanup.stats.removed_duplicate_triangles == 0 ||
        cleanup.stats.removed_degenerate_triangles == 0 ||
        after.duplicate_triangle_count >= before.duplicate_triangle_count ||
        after.degenerate_triangle_count >= before.degenerate_triangle_count) {
      std::cerr << "cleanup did not reduce duplicate/degenerate counts\n";
      return 1;
    }

    adasdf::TriangleMesh with_unused;
    with_unused.vertices = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {9.0, 9.0, 9.0}};
    with_unused.triangles.push_back({0, 1, 2, 0});
    const auto unused_cleanup = adasdf::MeshCleanup::clean(with_unused);
    if (!unused_cleanup.success ||
        unused_cleanup.stats.removed_unused_vertices != 1 ||
        unused_cleanup.cleaned_mesh.vertexCount() != 3) {
      std::cerr << "unused vertex cleanup failed\n";
      return 1;
    }

    adasdf::MeshCleanupOptions options;
    options.remove_duplicate_triangles = false;
    const auto no_duplicates = adasdf::MeshCleanup::clean(mesh, options);
    if (!no_duplicates.success ||
        no_duplicates.stats.removed_duplicate_triangles != 0) {
      std::cerr << "cleanup option did not disable duplicate removal\n";
      return 1;
    }

    std::cout << "mesh cleanup passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_mesh_cleanup failed: " << exc.what() << "\n";
    return 1;
  }
}

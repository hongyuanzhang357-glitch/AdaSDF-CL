#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const adasdf::STLReadResult read = adasdf::STLReader::read(fixture.string());
    if (!read.success) {
      std::cerr << "failed to read cube fixture\n";
      return 1;
    }
    adasdf::TriangleBVHBuildReport report;
    const adasdf::TriangleBVH bvh =
        adasdf::TriangleBVHBuilder::build(read.mesh, {}, &report);
    if (!bvh.isValid() || !report.success || bvh.nodeCount() == 0 ||
        bvh.leafCount() == 0 || bvh.triangleCount() != read.mesh.triangleCount()) {
      std::cerr << "BVH builder failed\n";
      return 1;
    }
    std::cout << "triangle BVH builder passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_triangle_bvh_builder failed: " << exc.what() << "\n";
    return 1;
  }
}

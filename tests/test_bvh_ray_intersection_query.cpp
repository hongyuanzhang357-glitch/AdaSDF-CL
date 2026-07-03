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
    const adasdf::TriangleBVH bvh = adasdf::TriangleBVHBuilder::build(read.mesh);
    const adasdf::BVHRay ray{{0.23, 0.37, 0.5}, {1.0, 0.0, 0.0}};
    const adasdf::BVHRayIntersectionResult result =
        adasdf::BVHRayIntersectionQuery::countIntersections(bvh, ray);
    if (!result.success || result.hit_count == 0 ||
        result.node_visits == 0 || result.triangle_tests == 0) {
      std::cerr << "ray intersection query failed\n";
      return 1;
    }
    std::cout << "BVH ray intersection query passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_bvh_ray_intersection_query failed: " << exc.what()
              << "\n";
    return 1;
  }
}

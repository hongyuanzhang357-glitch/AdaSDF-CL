#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

namespace {

bool near(double a, double b, double eps = 1.0e-9) {
  return std::abs(a - b) <= eps;
}

}  // namespace

int main() {
  try {
    adasdf::TriangleMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{0, 1, 2, 0}};
    const adasdf::TriangleBVH bvh = adasdf::TriangleBVHBuilder::build(mesh);
    const adasdf::BVHNearestTriangleQueryResult result =
        adasdf::BVHNearestTriangleQuery::query(bvh, {0.25, 0.25, 2.0});
    if (!result.success || result.triangle_index != 0 ||
        !near(result.distance, 2.0) || result.triangle_tests != 1) {
      std::cerr << "nearest triangle query failed\n";
      return 1;
    }
    std::cout << "BVH nearest triangle query passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_bvh_nearest_triangle_query failed: " << exc.what()
              << "\n";
    return 1;
  }
}

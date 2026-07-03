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
    const adasdf::Vector3 a{0.0, 0.0, 0.0};
    const adasdf::Vector3 b{1.0, 0.0, 0.0};
    const adasdf::Vector3 c{0.0, 2.0, 0.0};
    const adasdf::TriangleAABB box = adasdf::makeTriangleAABB(a, b, c);
    if (!adasdf::isValid(box) || !near(box.max.x, 1.0) ||
        !near(box.max.y, 2.0)) {
      std::cerr << "triangle AABB bounds failed\n";
      return 1;
    }
    if (!near(adasdf::squaredDistanceToPoint(box, {0.5, 0.5, 3.0}), 9.0)) {
      std::cerr << "AABB point distance failed\n";
      return 1;
    }
    const adasdf::TriangleAABB merged =
        adasdf::mergeTriangleAABB(box, adasdf::makeTriangleAABB(
                                           {2.0, 0.0, 0.0},
                                           {3.0, 0.0, 0.0},
                                           {2.0, 1.0, 0.0}));
    if (!adasdf::isValid(merged) || !near(merged.max.x, 3.0)) {
      std::cerr << "AABB merge failed\n";
      return 1;
    }
    std::cout << "triangle AABB passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_triangle_aabb failed: " << exc.what() << "\n";
    return 1;
  }
}

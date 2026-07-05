#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  const adasdf::Vector3 a{0.0, 0.0, 0.0};
  const adasdf::Vector3 b{1.0, 0.0, 0.0};
  const adasdf::Vector3 c{0.0, 1.0, 0.0};

  const adasdf::AABB far{{2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}, true};
  const adasdf::BoxTriangleDistanceResult far_result =
      adasdf::BoxTriangleDistance::estimate(far, a, b, c);
  if (!(far_result.lower_bound_distance > 1.0) ||
      !(far_result.approximate_distance > 1.0) ||
      far_result.intersects) {
    std::cerr << "far box should have positive box-triangle distance\n";
    return 1;
  }

  const adasdf::AABB crossing{{0.25, 0.25, -0.1}, {0.75, 0.75, 0.1}, true};
  const adasdf::BoxTriangleDistanceResult crossing_result =
      adasdf::BoxTriangleDistance::estimate(crossing, a, b, c);
  if (crossing_result.lower_bound_distance != 0.0 ||
      crossing_result.approximate_distance > 1.0e-9 ||
      !crossing_result.intersects) {
    std::cerr << "box crossing the triangle should be near zero distance\n";
    return 1;
  }

  std::cout << "box-triangle distance passed\n";
  return 0;
}

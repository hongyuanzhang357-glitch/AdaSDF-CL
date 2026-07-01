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
    const adasdf::Vector3 c{0.0, 1.0, 0.0};
    if (!near(adasdf::pointTriangleDistance({0.25, 0.25, 1.0}, a, b, c), 1.0)) {
      std::cerr << "interior projection distance failed\n";
      return 1;
    }
    if (!near(adasdf::pointTriangleDistance({0.5, -1.0, 0.0}, a, b, c), 1.0)) {
      std::cerr << "edge distance failed\n";
      return 1;
    }
    if (!near(
            adasdf::pointTriangleDistance({2.0, 0.0, 0.0}, a, b, c),
            1.0)) {
      std::cerr << "vertex distance failed\n";
      return 1;
    }
    const double degenerate =
        adasdf::pointTriangleDistance({0.0, 1.0, 0.0}, a, a, b);
    if (!std::isfinite(degenerate) || !near(degenerate, 1.0)) {
      std::cerr << "degenerate triangle fallback failed\n";
      return 1;
    }
    std::cout << "triangle distance passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_triangle_distance failed: " << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

bool near(double a, double b, double eps = 1.0e-9) {
  return std::abs(a - b) <= eps;
}

}  // namespace

int main() {
  try {
    auto model = adasdf::AnalyticSDFModel::createBox();
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "analytic model is not valid/queryable\n";
      return 1;
    }

    if (!(model->sampleDistance({0.0, 0.0, 0.0}) < 0.0)) {
      std::cerr << "box center distance should be negative\n";
      return 1;
    }
    if (!(model->sampleDistance({1.0, 0.0, 0.0}) > 0.0)) {
      std::cerr << "outside point distance should be positive\n";
      return 1;
    }
    if (!near(model->sampleDistance({0.5, 0.0, 0.0}), 0.0, 1.0e-12)) {
      std::cerr << "surface point distance should be near zero\n";
      return 1;
    }
    const adasdf::Vector3 gradient = model->sampleGradient({0.75, 0.0, 0.0});
    if (!gradient.allFinite() || gradient.x <= 0.0) {
      std::cerr << "outside gradient should be finite and point outward\n";
      return 1;
    }
    const adasdf::AABB bounds = model->boundingBox();
    if (!bounds.valid || !near(bounds.min.x, -0.5) || !near(bounds.max.x, 0.5) ||
        !near(bounds.min.y, -0.5) || !near(bounds.max.y, 0.5) ||
        !near(bounds.min.z, -0.5) || !near(bounds.max.z, 0.5)) {
      std::cerr << "analytic box AABB is incorrect\n";
      return 1;
    }

    const auto sphere = adasdf::AnalyticSDFModel::createSphere(
        {1.0, 2.0, 3.0}, 0.75);
    if (!near(sphere->sampleDistance({1.0, 2.0, 3.0}), -0.75) ||
        !near(sphere->sampleDistance({1.75, 2.0, 3.0}), 0.0) ||
        !near(sphere->sampleDistance({2.0, 2.0, 3.0}), 0.25) ||
        sphere->shapeName() != "sphere") {
      std::cerr << "analytic sphere signed distance is incorrect\n";
      return 1;
    }
    const adasdf::Vector3 sphere_gradient =
        sphere->sampleGradient({2.0, 2.0, 3.0});
    if (!sphere_gradient.allFinite() || sphere_gradient.x < 0.99) {
      std::cerr << "analytic sphere gradient is incorrect\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_analytic_sdf_model failed: " << exc.what() << "\n";
    return 1;
  }
}

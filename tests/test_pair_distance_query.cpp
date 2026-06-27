#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

namespace {

bool finiteVector(const adasdf::Vector3& value) {
  return value.allFinite();
}

}  // namespace

int main() {
  const std::filesystem::path fixture = ADASDF_CL_TEST_FIXTURE;
  if (fixture.empty()) {
    std::cout << "SKIP: no .sdfbin fixture discovered\n";
    return 0;
  }
  if (!adasdf::ExistingSDFBridge::existingCoreAvailable()) {
    std::cout << "SKIP: existing SDF core is not available\n";
    return 0;
  }

  try {
    auto model = adasdf::SDFBinReader::read(fixture);
    adasdf::CollisionObject obj_a(model);
    adasdf::CollisionObject obj_b(model);
    obj_b.setTransform(adasdf::Transform::fromTranslation({0.01, 0.0, 0.0}));

    adasdf::DistanceRequest request;
    request.backend = adasdf::BackendType::CPU;
    request.query_mode = adasdf::QueryMode::Balanced;
    request.enable_nearest_points = true;
    request.enable_gradient = true;

    adasdf::DistanceResult result;
    const double value = adasdf::distance(obj_a, obj_b, request, result);

    if (!std::isfinite(value) || !result.hasResult()) {
      std::cerr << "distance() did not return a finite result\n";
      return 1;
    }
    if (!finiteVector(result.nearestPointA()) || !finiteVector(result.nearestPointB()) ||
        !finiteVector(result.normal())) {
      std::cerr << "DistanceResult contains non-finite vectors\n";
      return 1;
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_pair_distance_query failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

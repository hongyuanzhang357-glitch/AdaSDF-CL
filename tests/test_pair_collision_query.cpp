#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

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

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 16;
    request.contact_tolerance = 1.0e-4;
    request.enable_gradient = true;
    request.backend = adasdf::BackendType::CPU;

    adasdf::CollisionResult result;
    const bool hit = adasdf::collide(obj_a, obj_b, request, result);

    if (!std::isfinite(result.minimumDistance())) {
      std::cerr << "collide() produced a non-finite minimum distance\n";
      return 1;
    }
    if (hit != result.isColliding()) {
      std::cerr << "collide() return value disagrees with CollisionResult\n";
      return 1;
    }
    for (const adasdf::Contact& contact : result.contacts()) {
      if (!contact.normal.allFinite() || !contact.point.allFinite()) {
        std::cerr << "contact contains non-finite point or normal\n";
        return 1;
      }
      if (contact.penetration_depth < 0.0 ||
          !std::isfinite(contact.penetration_depth)) {
        std::cerr << "contact penetration depth is invalid\n";
        return 1;
      }
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_pair_collision_query failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

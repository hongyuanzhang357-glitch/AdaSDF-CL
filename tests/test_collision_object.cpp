#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

namespace {

bool fail(const char* message) {
  std::cerr << message << "\n";
  return false;
}

bool finiteAABB(const adasdf::AABB& bounds) {
  return bounds.valid && bounds.min.allFinite() && bounds.max.allFinite() &&
         bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
         bounds.min.z <= bounds.max.z;
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
    adasdf::CollisionObject object(model);
    object.setTransform(adasdf::Transform::fromTranslation({0.25, -0.5, 0.75}));

    const adasdf::AABB local = model->boundingBox();
    const adasdf::AABB world = object.getAABB();
    if (!finiteAABB(local) || !finiteAABB(world)) {
      return fail("CollisionObject returned an invalid AABB") ? 0 : 1;
    }

    const adasdf::Vector3 p{1.0, 2.0, 3.0};
    const adasdf::Vector3 world_p = object.getTransform().applyPoint(p);
    const adasdf::Vector3 roundtrip = object.getTransform().inverseApplyPoint(world_p);
    if (std::abs(roundtrip.x - p.x) > 1.0e-9 ||
        std::abs(roundtrip.y - p.y) > 1.0e-9 ||
        std::abs(roundtrip.z - p.z) > 1.0e-9) {
      return fail("Transform inverseApplyPoint did not invert applyPoint") ? 0 : 1;
    }

    if (!object.getModel() || object.id() < 0) {
      return fail("CollisionObject did not retain model or id") ? 0 : 1;
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_collision_object failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

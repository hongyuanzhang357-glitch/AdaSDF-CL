#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

bool near(double a, double b, double eps = 1.0e-9) {
  return std::abs(a - b) <= eps;
}

}  // namespace

int main() {
  const adasdf::WorldTransform transform =
      adasdf::WorldTransform::fromQuaternionTranslation(
          adasdf::WorldQuaternion{0.0, 0.0, 0.0, 1.0},
          adasdf::Vector3{1.0, 2.0, 3.0});
  const adasdf::Vector3 p = transform.applyPoint({0.5, 0.0, 0.0});
  if (!near(p.x, 0.5) || !near(p.y, 2.0) || !near(p.z, 3.0)) {
    std::cerr << "world transform quaternion translation failed\n";
    return 1;
  }
  const adasdf::Vector3 local = transform.inverseApplyPoint(p);
  if (!near(local.x, 0.5) || !near(local.y, 0.0) || !near(local.z, 0.0)) {
    std::cerr << "world transform inverse failed\n";
    return 1;
  }
  const adasdf::AABB box{{-1.0, -2.0, -3.0}, {1.0, 2.0, 3.0}, true};
  const adasdf::AABB world_box = transform.applyAABB(box);
  if (!world_box.valid || !near(world_box.min.x, 0.0) ||
      !near(world_box.max.x, 2.0)) {
    std::cerr << "world transform AABB failed\n";
    return 1;
  }
  std::cout << "world transform passed\n";
  return 0;
}

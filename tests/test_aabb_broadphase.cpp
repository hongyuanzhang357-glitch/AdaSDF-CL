#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::WorldObject makeObject(
    int id,
    double tx,
    adasdf::WorldObjectType type = adasdf::WorldObjectType::Dynamic) {
  adasdf::WorldObject object;
  object.object_id = id;
  object.name = "box_" + std::to_string(id);
  object.model = adasdf::AnalyticSDFModel::createBox(
      adasdf::Vector3::Zero(),
      {0.5, 0.5, 0.5});
  object.transform =
      adasdf::WorldTransform::fromTranslation({tx, 0.0, 0.0});
  object.type = type;
  return object;
}

}  // namespace

int main() {
  adasdf::CollisionWorld world;
  world.addObject(makeObject(0, 0.0));
  world.addObject(makeObject(1, 0.75));
  world.addObject(makeObject(2, 4.0));

  const adasdf::AABBBroadphaseResult result =
      adasdf::AABBBroadphase::compute(world);
  if (!result.success || result.pairs.size() != 1 ||
      result.pairs[0].object_a != 0 || result.pairs[0].object_b != 1) {
    std::cerr << "AABB broadphase overlap filtering failed\n";
    return 1;
  }

  adasdf::CollisionWorld static_world;
  static_world.addObject(makeObject(0, 0.0, adasdf::WorldObjectType::Static));
  static_world.addObject(makeObject(1, 0.0, adasdf::WorldObjectType::Static));
  if (!adasdf::AABBBroadphase::compute(static_world).pairs.empty()) {
    std::cerr << "static-static filtering failed\n";
    return 1;
  }
  adasdf::AABBBroadphaseOptions include_static;
  include_static.include_static_static = true;
  if (adasdf::AABBBroadphase::compute(static_world, include_static).pairs.size() != 1) {
    std::cerr << "static-static include option failed\n";
    return 1;
  }
  std::cout << "AABB broadphase passed\n";
  return 0;
}

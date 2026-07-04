#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::WorldObject makeObject(int id, double tx) {
  adasdf::WorldObject object;
  object.object_id = id;
  object.name = "object_" + std::to_string(id);
  object.model = adasdf::AnalyticSDFModel::createBox(
      adasdf::Vector3::Zero(),
      {0.5, 0.5, 0.5});
  object.transform =
      adasdf::WorldTransform::fromTranslation({tx, 0.0, 0.0});
  object.type = adasdf::WorldObjectType::Dynamic;
  return object;
}

}  // namespace

int main() {
  adasdf::CollisionWorld world;
  world.addObject(makeObject(4, 0.0));
  world.addObject(makeObject(2, 0.25));
  if (world.objectCount() != 2 ||
      world.objects().front().object_id != 2 ||
      !world.findObject(4)) {
    std::cerr << "world add/find deterministic ordering failed\n";
    return 1;
  }
  if (!world.updateTransform(4, adasdf::WorldTransform::fromTranslation({2, 0, 0}))) {
    std::cerr << "world transform update failed\n";
    return 1;
  }
  if (!world.findObject(4)->world_aabb.valid ||
      world.findObject(4)->world_aabb.min.x < 1.4) {
    std::cerr << "world AABB refresh failed\n";
    return 1;
  }
  if (!world.removeObject(2) || world.objectCount() != 1) {
    std::cerr << "world remove failed\n";
    return 1;
  }
  std::cout << "collision world passed\n";
  return 0;
}

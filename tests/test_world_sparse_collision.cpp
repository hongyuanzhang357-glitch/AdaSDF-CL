#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::WorldObject makeObject(int id, double tx) {
  adasdf::WorldObject object;
  object.object_id = id;
  object.name = "box_" + std::to_string(id);
  object.model = adasdf::AnalyticSDFModel::createBox(
      adasdf::Vector3::Zero(),
      {0.5, 0.5, 0.5});
  object.transform =
      adasdf::WorldTransform::fromTranslation({tx, 0.0, 0.0});
  object.type = adasdf::WorldObjectType::Dynamic;
  adasdf::CollisionSample sample;
  sample.sample_id = id * 10;
  sample.position = {0.0, 0.0, 0.0};
  sample.radius = 0.0;
  sample.label = "center";
  object.samples.add(sample);
  object.has_samples = true;
  return object;
}

}  // namespace

int main() {
  adasdf::CollisionWorld world;
  world.addObject(makeObject(0, 0.0));
  world.addObject(makeObject(1, 0.25));

  adasdf::WorldSparseCollisionOptions options;
  options.mode = adasdf::SparseCollisionMode::CandidateSearch;
  options.threshold = 0.0;
  options.early_exit = false;
  options.compute_normals = true;
  const adasdf::WorldSparseCollisionResult result =
      adasdf::WorldSparseCollision::check(world, options);
  if (!result.success || !result.colliding ||
      result.stats.broadphase_pair_count != 1 ||
      result.stats.violation_count == 0) {
    std::cerr << "world sparse collision failed: "
              << result.error_message << "\n";
    return 1;
  }
  if (result.pairs[0].violations[0].source_object_id ==
      result.pairs[0].violations[0].target_object_id) {
    std::cerr << "world sparse source/target metadata failed\n";
    return 1;
  }
  std::cout << "world sparse collision passed\n";
  return 0;
}

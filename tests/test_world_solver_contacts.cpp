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
  adasdf::CollisionSample sample;
  sample.sample_id = id * 10;
  sample.position = {0.0, 0.0, 0.0};
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

  adasdf::WorldSolverContactOptions options;
  options.collision_options.threshold = 0.0;
  options.candidate_options.top_k = 8;
  options.stabilization_options.budget.max_contacts_total = 4;
  const adasdf::WorldSolverContactResult result =
      adasdf::WorldSolverContacts::build(world, options);
  if (!result.success || result.contacts.empty()) {
    std::cerr << "world solver contacts failed: "
              << result.error_message << "\n";
    return 1;
  }
  if (result.contacts.contacts[0].stable_key.empty() ||
      result.contacts.contacts[0].group_id < 0) {
    std::cerr << "world solver contact stable metadata failed\n";
    return 1;
  }
  std::cout << "world solver contacts passed\n";
  return 0;
}

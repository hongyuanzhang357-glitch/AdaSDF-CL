#include <adasdf/adasdf.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

struct Scenario {
  const char* name;
  adasdf::Vector3 offset;
};

void runScenario(
    const Scenario& scenario,
    const std::shared_ptr<adasdf::SDFModel>& model) {
  using namespace adasdf;

  CollisionObject obj_a(model);
  CollisionObject obj_b(model);
  obj_b.setTransform(Transform::fromTranslation(scenario.offset));

  CollisionRequest collision_request;
  collision_request.enable_contact = true;
  collision_request.max_contacts = 4;
  collision_request.contact_tolerance = 1.0e-4;
  collision_request.query_mode = QueryMode::Balanced;
  collision_request.enable_gradient = true;
  collision_request.enable_contact_reduction = true;

  DistanceRequest distance_request;
  distance_request.query_mode = QueryMode::Balanced;
  distance_request.enable_gradient = true;

  DistanceResult distance_result;
  const double distance_value =
      distance(obj_a, obj_b, distance_request, distance_result);

  CollisionResult collision_result;
  const bool hit = collide(obj_a, obj_b, collision_request, collision_result);

  std::cout << "\nScenario: " << scenario.name << "\n";
  printVec("Offset: ", scenario.offset);
  std::cout << "Query mode: Balanced\n";
  std::cout << "Backend: " << collision_result.backendInfo() << "\n";
  std::cout << "Method: " << collision_result.methodInfo() << "\n";
  std::cout << "Distance: " << distance_value << "\n";
  std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
  std::cout << "Candidate points: " << collision_result.numCandidatePoints()
            << "\n";
  std::cout << "Raw contacts: " << collision_result.numRawContacts() << "\n";
  std::cout << "Reduced contacts: " << collision_result.numReducedContacts()
            << "\n";
  std::cout << "SDF queries: " << collision_result.numSDFQueries() << "\n";
  std::cout << "Distance method: " << distance_result.methodInfo() << "\n";

  int index = 0;
  for (const Contact& contact : collision_result.contacts()) {
    std::cout << "Contact[" << index << "].signed_distance: "
              << contact.signed_distance << "\n";
    std::cout << "Contact[" << index << "].penetration_depth: "
              << contact.penetration_depth << "\n";
    printVec(("Contact[" + std::to_string(index) + "].normal: ").c_str(),
             contact.normal);
    ++index;
  }
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 2) {
    std::cout << "Usage: adasdf_contact_reduction_demo model.sdfbin\n";
    std::cout << "If you do not have a model yet, build one first:\n";
    std::cout << "  adasdf_build adasdf_cl/tests/data/cube_closed_ascii.stl "
                 "build/cube.sdfbin\n";
    return 0;
  }

  try {
    auto model = SDFBinReader::read(std::filesystem::path(argv[1]));
    const std::array<Scenario, 3> scenarios = {{
        {"separated", {1.25, 0.0, 0.0}},
        {"near_contact", {1.0, 0.0, 0.0}},
        {"penetrating", {0.25, 0.0, 0.0}},
    }};

    std::cout << "AdaSDF-CL contact reduction demo\n";
    std::cout << "Model: " << argv[1] << "\n";
    for (const Scenario& scenario : scenarios) {
      runScenario(scenario, model);
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "AdaSDF-CL contact reduction demo failed: " << exc.what()
              << "\n";
    return 1;
  }
}

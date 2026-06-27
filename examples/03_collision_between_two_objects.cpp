#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

void printAABB(const char* label, const adasdf::AABB& aabb) {
  std::cout << label;
  if (!aabb.valid) {
    std::cout << "invalid\n";
    return;
  }
  std::cout << "min=(" << aabb.min << ") max=(" << aabb.max << ")\n";
}

adasdf::Vector3 parseOffset(int argc, char** argv) {
  if (argc >= 6) {
    return {std::stod(argv[3]), std::stod(argv[4]), std::stod(argv[5])};
  }
  return {0.01, 0.0, 0.0};
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 2) {
    std::cout << "Usage: adasdf_collision_between_two_objects object_a.sdfbin "
                 "[object_b.sdfbin] [dx dy dz]\n";
    return 0;
  }

  try {
    const std::filesystem::path path_a = argv[1];
    const bool has_b_path = argc >= 3 && std::filesystem::exists(argv[2]);
    const std::filesystem::path path_b = has_b_path ? std::filesystem::path(argv[2]) : path_a;
    const Vector3 offset = has_b_path ? parseOffset(argc, argv) : Vector3{0.01, 0.0, 0.0};

    auto model_a = SDFBinReader::read(path_a);
    auto model_b = has_b_path ? SDFBinReader::read(path_b) : model_a;

    CollisionObject obj_a(model_a);
    CollisionObject obj_b(model_b);
    obj_a.setTransform(Transform::Identity());
    obj_b.setTransform(Transform::fromTranslation(offset));

    CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 32;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = QueryMode::Balanced;
    request.backend = BackendType::CPU;
    request.enable_gradient = true;

    CollisionResult result;
    const bool hit = collide(obj_a, obj_b, request, result);

    std::cout << "AdaSDF-CL pair collision example\n";
    std::cout << "Model A: " << path_a.string() << "\n";
    std::cout << "Model B: " << path_b.string() << "\n";
    printAABB("AABB A: ", obj_a.getAABB());
    printAABB("AABB B: ", obj_b.getAABB());
    std::cout << "Query mode: Balanced\n";
    std::cout << "Backend: " << result.backendInfo() << "\n";
    std::cout << "Method: " << result.methodInfo() << "\n";
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << result.minimumDistance() << "\n";
    std::cout << "Candidate points: " << result.numCandidatePoints() << "\n";
    std::cout << "Raw contacts: " << result.numRawContacts() << "\n";
    std::cout << "Reduced contacts: " << result.numReducedContacts() << "\n";
    std::cout << "Contact count: " << result.contacts().size() << "\n";
    if (!result.contacts().empty()) {
      const Contact& c = result.contacts().front();
      printVec("Contact[0].point: ", c.point);
      printVec("Contact[0].normal: ", c.normal);
      std::cout << "Contact[0].penetration_depth: " << c.penetration_depth << "\n";
    }
    std::cout << "Number of SDF queries: " << result.numSDFQueries() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "AdaSDF-CL pair collision example failed: " << exc.what() << "\n";
    return 1;
  }
}

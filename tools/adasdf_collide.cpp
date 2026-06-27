#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_collide object_a.sdfbin object_b.sdfbin [options]\n"
      << "Options:\n"
      << "  --offset dx dy dz       World-space translation applied to object B\n"
      << "  --max-contacts value    Maximum contacts to report, default 32\n"
      << "  --tolerance value       Contact tolerance, default 1e-4\n"
      << "  --grid-resolution value Candidate grid resolution, default 8\n"
      << "  --no-contact            Only report collision state and distance\n";
}

bool hasValue(int index, int argc, int count = 1) {
  return index + count < argc;
}

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

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc < 3) {
    usage();
    return 2;
  }

  const std::filesystem::path path_a = argv[1];
  const std::filesystem::path path_b = argv[2];
  Vector3 offset{0.0, 0.0, 0.0};

  CollisionRequest request;
  request.enable_contact = true;
  request.max_contacts = 32;
  request.contact_tolerance = 1.0e-4;
  request.enable_gradient = true;
  request.backend = BackendType::CPU;
  request.query_mode = QueryMode::Balanced;

  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--offset" && hasValue(i, argc, 3)) {
      offset = {std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--max-contacts" && hasValue(i, argc)) {
      request.max_contacts = std::stoi(argv[++i]);
    } else if (arg == "--tolerance" && hasValue(i, argc)) {
      request.contact_tolerance = std::stod(argv[++i]);
    } else if (arg == "--grid-resolution" && hasValue(i, argc)) {
      request.candidate_grid_resolution = std::stoi(argv[++i]);
    } else if (arg == "--no-contact") {
      request.enable_contact = false;
      request.max_contacts = 0;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  if (!std::filesystem::exists(path_a)) {
    std::cerr << "adasdf_collide: file does not exist: " << path_a.string() << "\n";
    return 2;
  }
  if (!std::filesystem::exists(path_b)) {
    std::cerr << "adasdf_collide: file does not exist: " << path_b.string() << "\n";
    return 2;
  }

  try {
    const auto model_a = SDFBinReader::read(path_a);
    const auto model_b = SDFBinReader::read(path_b);

    CollisionObject object_a(model_a);
    CollisionObject object_b(model_b);
    object_a.setTransform(Transform::Identity());
    object_b.setTransform(Transform::fromTranslation(offset));

    CollisionResult result;
    const bool hit = collide(object_a, object_b, request, result);

    std::cout << "AdaSDF-CL collision query\n";
    std::cout << "Model A: " << path_a.string() << "\n";
    std::cout << "Model B: " << path_b.string() << "\n";
    printVec("Offset B: ", offset);
    printAABB("AABB A: ", object_a.getAABB());
    printAABB("AABB B: ", object_b.getAABB());
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << result.minimumDistance() << "\n";
    std::cout << "Backend: " << result.backendInfo() << "\n";
    std::cout << "Method: " << result.methodInfo() << "\n";
    std::cout << "Candidate points: " << result.numCandidatePoints() << "\n";
    std::cout << "Raw contacts: " << result.numRawContacts() << "\n";
    std::cout << "Reduced contacts: " << result.numReducedContacts() << "\n";
    std::cout << "Contact count: " << result.contacts().size() << "\n";
    if (!result.contacts().empty()) {
      const Contact& contact = result.contacts().front();
      printVec("Contact[0].point: ", contact.point);
      printVec("Contact[0].normal: ", contact.normal);
      std::cout << "Contact[0].penetration_depth: "
                << contact.penetration_depth << "\n";
      std::cout << "Contact[0].signed_distance: "
                << contact.signed_distance << "\n";
    }
    std::cout << "Number of SDF queries: " << result.numSDFQueries() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_collide failed: " << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_collide_boxes_demo [options]\n"
      << "Options:\n"
      << "  --target-error value       Target near-surface error, default 1e-3\n"
      << "  --memory-mb value          Memory limit, default 64\n"
      << "  --block-memory-mb value    Block expansion memory limit, default 16\n"
      << "  --offset dx dy dz          Translation for box B, default 0.25 0 0\n"
      << "  --max-contacts value       Maximum contacts, default 8\n"
      << "  --view path.svg            Write SVG collision view\n";
}

bool hasValue(int index, int argc, int count = 1) {
  return index + count < argc;
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  DemoAdaptiveBuildRequest build_request;
  build_request.use_surrogate = true;
  Vector3 offset{0.25, 0.0, 0.0};
  int max_contacts = 8;
  std::filesystem::path view_path;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--target-error" && hasValue(i, argc)) {
      build_request.target_near_surface_error = std::stod(argv[++i]);
    } else if (arg == "--memory-mb" && hasValue(i, argc)) {
      build_request.memory_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--block-memory-mb" && hasValue(i, argc)) {
      build_request.block_expand_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--offset" && hasValue(i, argc, 3)) {
      offset = {std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--max-contacts" && hasValue(i, argc)) {
      max_contacts = std::stoi(argv[++i]);
    } else if (arg == "--view" && hasValue(i, argc)) {
      view_path = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  try {
    DemoAdaptiveBuildResult build = DemoAdaptiveSDFBuilder::build(build_request);
    CollisionObject object_a(build.model);
    CollisionObject object_b(build.model);
    object_b.setTransform(Transform::fromTranslation(offset));

    CollisionRequest request;
    request.enable_contact = true;
    request.enable_gradient = true;
    request.max_contacts = max_contacts;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = QueryMode::Balanced;

    CollisionResult result;
    const bool hit = collide(object_a, object_b, request, result);

    std::cout << "AdaSDF-CL two-box demo collision\n";
    std::cout << "Surrogate: " << DemoSurrogateRecommender::id()
              << " experimental demo only\n";
    std::cout << "Warning: " << DemoSurrogateRecommender::statusWarning()
              << "\n";
    printVec("Offset B: ", offset);
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << result.minimumDistance() << "\n";
    std::cout << "Backend: " << result.backendInfo() << "\n";
    std::cout << "Method: " << result.methodInfo() << "\n";
    std::cout << "Requested max contacts: " << max_contacts << "\n";
    std::cout << "Returned contacts: " << result.contacts().size() << "\n";
    for (std::size_t i = 0; i < result.contacts().size(); ++i) {
      const Contact& contact = result.contacts()[i];
      std::cout << "Contact[" << i << "]\n";
      printVec("  point: ", contact.point);
      printVec("  normal: ", contact.normal);
      std::cout << "  penetration_depth: "
                << contact.penetration_depth << "\n";
    }

    if (!view_path.empty()) {
      CollisionSVGScene scene;
      scene.box_a = object_a.getAABB();
      scene.box_b = object_b.getAABB();
      scene.result = result;
      scene.title = "AdaSDF-CL v0.9 demo collision";
      scene.backend = result.backendInfo();
      scene.method = result.methodInfo();
      scene.requested_max_contacts = max_contacts;
      CollisionSVGWriter::write(view_path, scene);
      std::cout << "SVG view: " << view_path.string() << "\n";
    }
    return result.contacts().size() <= static_cast<std::size_t>(max_contacts) ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_collide_boxes_demo failed: " << exc.what() << "\n";
    return 1;
  }
}

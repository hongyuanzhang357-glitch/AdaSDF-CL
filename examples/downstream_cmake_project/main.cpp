#include <adasdf/adasdf.h>

#include <exception>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {

void printAABB(const adasdf::AABB& aabb) {
  if (!aabb.valid) {
    std::cout << "AABB: invalid\n";
    return;
  }
  std::cout << "AABB min: " << aabb.min << "\n";
  std::cout << "AABB max: " << aabb.max << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::cout << "AdaSDF-CL downstream example\n";
  std::cout << "Version: " << adasdf::versionString() << "\n";

  const auto backend = adasdf::makeBackend(adasdf::BackendType::CPU);
  std::cout << "CPU backend: "
            << (backend && backend->available() ? "available" : "unavailable")
            << "\n";

  if (argc == 1) {
    adasdf::DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = true;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);
    const auto model = build.model;
    const double phi = model->sampleDistance({0.0, 0.0, 0.0});

    adasdf::CollisionObject a(model);
    adasdf::CollisionObject b(model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    adasdf::CollisionResult result;
    const bool hit = adasdf::collide(a, b, request, result);

    std::cout << "No .sdfbin supplied; running core-free demo adaptive path.\n";
    std::cout << "Demo signed distance at origin: " << phi << "\n";
    std::cout << "Demo adaptive blocks: " << model->blocks().size() << "\n";
    std::cout << "Demo colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Demo contacts: " << result.contacts().size() << "\n";
    return backend && std::isfinite(phi) && hit ? 0 : 1;
  }
  if (argc != 2) {
    std::cerr << "Usage: adasdf_downstream [model.sdfbin]\n";
    return 2;
  }

  try {
    const std::filesystem::path path = argv[1];
    const auto model = adasdf::SDFBinReader::read(path);
    std::cout << "Loaded: " << path.string() << "\n";
    std::cout << "Valid: " << (model->isValid() ? "true" : "false") << "\n";
    std::cout << "Query backend available: "
              << (model->queryBackendAvailable() ? "true" : "false") << "\n";
    printAABB(model->boundingBox());
    return model->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "downstream example failed: " << exc.what() << "\n";
    return 1;
  }
}

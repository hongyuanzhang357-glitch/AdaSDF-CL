#include <adasdf/adasdf.h>

#include <exception>
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
    std::cout << "No .sdfbin supplied; package integration smoke test complete.\n";
    return backend ? 0 : 1;
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

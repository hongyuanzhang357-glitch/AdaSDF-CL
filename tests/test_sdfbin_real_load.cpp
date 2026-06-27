#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

int main() {
  const std::filesystem::path fixture = ADASDF_CL_TEST_FIXTURE;
  if (fixture.empty()) {
    std::cout << "SKIP: no .sdfbin fixture discovered under models/.\n";
    return 0;
  }
  if (!adasdf::ExistingSDFBridge::existingCoreAvailable()) {
    std::cout << "SKIP: existing SDF core bridge is not available.\n";
    return 0;
  }

  auto model = adasdf::SDFBinReader::read(fixture);
  if (!model) {
    std::cerr << "model is null\n";
    return 1;
  }
  if (!model->isValid()) {
    std::cerr << "model is not valid\n";
    return 1;
  }
  const adasdf::AABB bounds = model->boundingBox();
  if (!bounds.valid ||
      !(bounds.min.x <= bounds.max.x) ||
      !(bounds.min.y <= bounds.max.y) ||
      !(bounds.min.z <= bounds.max.z)) {
    std::cerr << "invalid AABB\n";
    return 1;
  }
  if (model->memoryFootprintBytes() == 0) {
    std::cerr << "memory footprint was not populated\n";
    return 1;
  }
  return 0;
}

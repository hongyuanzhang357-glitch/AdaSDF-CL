#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_FIXTURE
#define ADASDF_CL_TEST_FIXTURE ""
#endif

namespace {

bool finite(const adasdf::Vector3& v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

}  // namespace

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
  const adasdf::AABB bounds = model->boundingBox();
  const adasdf::Vector3 p = 0.5 * (bounds.min + bounds.max);

  const double phi = model->sampleDistance(p);
  if (!std::isfinite(phi)) {
    std::cerr << "sampleDistance returned non-finite value\n";
    return 1;
  }

  const adasdf::Vector3 gradient = model->sampleGradient(p);
  if (!finite(gradient)) {
    std::cerr << "sampleGradient returned non-finite value\n";
    return 1;
  }
  return 0;
}

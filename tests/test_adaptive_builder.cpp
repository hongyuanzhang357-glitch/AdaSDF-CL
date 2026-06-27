#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_CUBE_STL
#define ADASDF_CL_TEST_CUBE_STL ""
#endif

namespace {

bool finite(const adasdf::Vector3& v) {
  return v.allFinite();
}

}  // namespace

int main() {
  if (!adasdf::ExistingBuilderBridge::isAvailable()) {
    std::cout << "SKIP: existing adaptive builder bridge is not available\n";
    return 0;
  }

  const std::filesystem::path cube = ADASDF_CL_TEST_CUBE_STL;
  if (cube.empty() || !std::filesystem::exists(cube)) {
    std::cout << "SKIP: cube STL fixture is not available\n";
    return 0;
  }

  try {
    adasdf::BuildOptions options;
    options.max_octree_level = 4;
    options.base_block_cells = 8;
    options.min_block_cells = 4;
    options.ghost_cells = 1;
    options.max_rank = 4;
    options.near_min_rank = 2;
    options.verbose = false;

    auto model = adasdf::AdaptiveSDFBuilder::fromMesh(cube.string(), options);
    if (!model || !model->isValid()) {
      std::cerr << "AdaptiveSDFBuilder returned invalid model\n";
      return 1;
    }

    const adasdf::AABB bounds = model->boundingBox();
    const adasdf::Vector3 p = 0.5 * (bounds.min + bounds.max);
    const double phi = model->sampleDistance(p);
    const adasdf::Vector3 grad = model->sampleGradient(p);
    if (!std::isfinite(phi) || !finite(grad)) {
      std::cerr << "Built model query returned non-finite values\n";
      return 1;
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_builder failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

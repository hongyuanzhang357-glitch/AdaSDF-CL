#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_CUBE_STL
#define ADASDF_CL_TEST_CUBE_STL ""
#endif

namespace {

adasdf::BuildOptions smallBuildOptions() {
  adasdf::BuildOptions options;
  options.max_octree_level = 4;
  options.base_block_cells = 8;
  options.min_block_cells = 4;
  options.ghost_cells = 1;
  options.max_rank = 4;
  options.near_min_rank = 2;
  options.verbose = false;
  return options;
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
    auto model =
        adasdf::AdaptiveSDFBuilder::fromSTL(cube.string(), smallBuildOptions());
    adasdf::CollisionObject object(model);

    adasdf::CandidateSamplingOptions options;
    options.grid_resolution = 6;
    options.max_points = 128;
    options.surface_band = 1.0e-3;

    const auto points =
        adasdf::CandidatePointSampler::sampleWorldPoints(object, options);
    if (points.empty() || points.size() > static_cast<std::size_t>(options.max_points)) {
      std::cerr << "candidate point count is outside expected bounds\n";
      return 1;
    }
    for (const adasdf::Vector3& point : points) {
      if (!point.allFinite()) {
        std::cerr << "candidate point is non-finite\n";
        return 1;
      }
    }

    const adasdf::Vector3 offset{0.25, -0.125, 0.5};
    adasdf::CollisionObject translated(model);
    translated.setTransform(adasdf::Transform::fromTranslation(offset));
    const auto shifted =
        adasdf::CandidatePointSampler::sampleWorldPoints(translated, options);
    if (shifted.size() != points.size()) {
      std::cerr << "translated candidate count changed unexpectedly\n";
      return 1;
    }
    for (std::size_t i = 0; i < points.size(); ++i) {
      const adasdf::Vector3 delta = shifted[i] - points[i];
      if (std::abs(delta.x - offset.x) > 1.0e-9 ||
          std::abs(delta.y - offset.y) > 1.0e-9 ||
          std::abs(delta.z - offset.z) > 1.0e-9) {
        std::cerr << "candidate points did not follow the object transform\n";
        return 1;
      }
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_candidate_point_sampler failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

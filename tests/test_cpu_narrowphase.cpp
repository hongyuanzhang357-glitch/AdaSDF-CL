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

    adasdf::CollisionObject separated_a(model);
    adasdf::CollisionObject separated_b(model);
    separated_b.setTransform(adasdf::Transform::fromTranslation({2.0, 0.0, 0.0}));

    adasdf::DistanceRequest distance_request;
    distance_request.query_mode = adasdf::QueryMode::Balanced;
    adasdf::DistanceResult distance_result;
    const double d =
        adasdf::distance(separated_a, separated_b, distance_request, distance_result);
    if (!std::isfinite(d) || d < 0.0) {
      std::cerr << "separated distance is not finite and non-negative\n";
      return 1;
    }

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 2;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = adasdf::QueryMode::Balanced;
    request.enable_gradient = true;

    adasdf::CollisionResult separated_collision;
    if (adasdf::collide(separated_a, separated_b, request, separated_collision)) {
      std::cerr << "separated objects reported collision\n";
      return 1;
    }

    adasdf::CollisionObject obj_a(model);
    adasdf::CollisionObject obj_b(model);
    obj_b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionResult collision_result;
    const bool hit = adasdf::collide(obj_a, obj_b, request, collision_result);
    if (!hit || collision_result.contacts().empty()) {
      std::cerr << "penetrating objects did not report contacts\n";
      return 1;
    }
    if (collision_result.contacts().size() >
        static_cast<std::size_t>(request.max_contacts)) {
      std::cerr << "max_contacts was not enforced\n";
      return 1;
    }
    if (collision_result.numSDFQueries() == 0 ||
        collision_result.backendInfo().find("CPU narrow-phase") ==
            std::string::npos) {
      std::cerr << "narrow-phase stats/backend info are missing\n";
      return 1;
    }
    if (collision_result.numCandidatePoints() == 0 ||
        collision_result.numRawContacts() == 0 ||
        collision_result.numReducedContacts() != collision_result.contacts().size()) {
      std::cerr << "narrow-phase contact statistics are inconsistent\n";
      return 1;
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_cpu_narrowphase failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

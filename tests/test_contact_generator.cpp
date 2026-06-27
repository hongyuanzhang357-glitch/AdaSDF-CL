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

double norm(const adasdf::Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
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
    adasdf::CollisionObject obj_a(model);
    adasdf::CollisionObject obj_b(model);
    obj_b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 16;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = adasdf::QueryMode::Balanced;
    request.max_candidate_points = 512;

    adasdf::NarrowPhaseStats stats;
    const auto raw = adasdf::ContactGenerator::generateSymmetricContacts(
        obj_a, obj_b, request, stats);

    if (raw.empty() || stats.num_sdf_queries == 0 ||
        stats.num_candidate_points == 0) {
      std::cerr << "contact generator did not produce expected samples\n";
      return 1;
    }
    for (const adasdf::RawContactCandidate& contact : raw) {
      if (!std::isfinite(contact.signed_distance) ||
          !contact.point_world.allFinite() || !contact.normal_world.allFinite()) {
        std::cerr << "raw contact contains non-finite data\n";
        return 1;
      }
      if (contact.penetration_depth < 0.0 ||
          !std::isfinite(contact.penetration_depth)) {
        std::cerr << "raw contact penetration depth is invalid\n";
        return 1;
      }
      if (norm(contact.normal_world) < 0.5 || norm(contact.normal_world) > 1.5) {
        std::cerr << "raw contact normal is not normalized enough\n";
        return 1;
      }
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_generator failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

bool finite(const adasdf::Vector3& v) {
  return v.allFinite();
}

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    build_options.max_octree_level = 2;
    build_options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        build_options,
        &build_report);
    auto adaptive = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
    adasdf::BlockLowRankCompressionOptions options;
    options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &report);
    auto compressed_model =
        std::make_shared<adasdf::CompressedAdaptiveBlockSDFModel>(std::move(compressed));
    if (!compressed_model->isValid()) {
      std::cerr << "compressed model invalid\n";
      return 1;
    }

    adasdf::CollisionObject a(compressed_model);
    adasdf::CollisionObject b(compressed_model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));
    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    adasdf::CollisionResult hit_result;
    if (!adasdf::collide(a, b, request, hit_result)) {
      std::cerr << "compressed cubes should collide\n";
      return 1;
    }
    if (hit_result.contacts().size() > 4) {
      std::cerr << "max contacts not enforced\n";
      return 1;
    }
    if (!hit_result.contacts().empty()) {
      const adasdf::Contact& contact = hit_result.contacts().front();
      if (!finite(contact.normal) || !std::isfinite(contact.penetration_depth)) {
        std::cerr << "contact output non-finite\n";
        return 1;
      }
    }

    adasdf::CollisionObject c(compressed_model);
    c.setTransform(adasdf::Transform::fromTranslation({2.0, 0.0, 0.0}));
    adasdf::CollisionResult far_result;
    const bool far_hit = adasdf::collide(a, c, request, far_result);
    if (far_hit || !(far_result.minimumDistance() > 0.0)) {
      std::cerr << "separated compressed cubes should have positive distance\n";
      return 1;
    }
    std::cout << "compressed sdf collision passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compressed_sdf_collision failed: " << exc.what() << "\n";
    return 1;
  }
}

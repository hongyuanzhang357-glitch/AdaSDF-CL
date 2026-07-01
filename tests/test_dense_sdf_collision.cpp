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
    adasdf::DenseSDFBuildOptions options;
    options.resolution = 20;
    options.padding = 0.05;
    adasdf::DenseSDFBuildReport report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    if (!model) {
      std::cerr << report.error_message << "\n";
      return 1;
    }

    adasdf::CollisionObject a(model);
    adasdf::CollisionObject b(model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));
    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    adasdf::CollisionResult hit_result;
    if (!adasdf::collide(a, b, request, hit_result)) {
      std::cerr << "dense cubes should collide\n";
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

    adasdf::CollisionObject c(model);
    c.setTransform(adasdf::Transform::fromTranslation({2.0, 0.0, 0.0}));
    adasdf::CollisionResult far_result;
    const bool far_hit = adasdf::collide(a, c, request, far_result);
    if (far_hit || !(far_result.minimumDistance() > 0.0)) {
      std::cerr << "separated dense cubes should have positive distance\n";
      return 1;
    }
    std::cout << "dense sdf collision passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_dense_sdf_collision failed: " << exc.what() << "\n";
    return 1;
  }
}

#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main() {
  try {
    std::cout << "AdaSDF-CL capability walkthrough\n";
    std::cout << "Version: " << adasdf::versionString() << "\n";

    adasdf::DemoAdaptiveBuildRequest request;
    request.use_surrogate = true;
    request.target_near_surface_error = 1.0e-3;
    request.memory_limit_mb = 64.0;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(request);
    if (!build.model) {
      std::cerr << "failed to build demo adaptive model\n";
      return 1;
    }

    std::cout << "Query backends: CPU direct, CPU expanded, optional CUDA expanded\n";
    const adasdf::Vector3 point{0.0, 0.0, 0.0};
    std::cout << "Point phi: " << build.model->sampleDistance(point) << "\n";
    printVec("Point normal: ", build.model->sampleNormal(point));

    adasdf::CollisionObject object_a(build.model);
    adasdf::CollisionObject object_b(build.model);
    object_b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionRequest collision_request;
    collision_request.enable_contact = true;
    collision_request.max_contacts = 4;
    adasdf::CollisionResult collision_result;
    const bool hit =
        adasdf::collide(object_a, object_b, collision_request, collision_result);
    std::cout << "Collision hit: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << collision_result.minimumDistance() << "\n";
    std::cout << "Raw contacts: " << collision_result.numRawContacts() << "\n";
    std::cout << "Reduced contacts: " << collision_result.numReducedContacts() << "\n";
    if (!collision_result.contacts().empty()) {
      const adasdf::Contact& contact = collision_result.contacts().front();
      printVec("Contact point: ", contact.point);
      printVec("Contact normal: ", contact.normal);
      std::cout << "Contact penetration depth: "
                << contact.penetration_depth << "\n";
      std::cout << "Contact signed distance: "
                << contact.signed_distance << "\n";
    }

    adasdf::ExpansionOptions expansion_options;
    expansion_options.expansion = adasdf::QueryExpansionMode::Global;
    expansion_options.global_resolution = 32;
    const adasdf::ExpandedSDF expanded =
        adasdf::SDFExpander::expand(*build.model, expansion_options);

    adasdf::ExpansionQualityOptions quality_options;
    quality_options.num_samples = 512;
    quality_options.near_surface_band = 1.0e-3;
    const adasdf::ExpansionQualityReport report =
        adasdf::ExpansionQuality::compareAgainstDirect(
            *build.model,
            expanded,
            quality_options);
    std::cout << "Expansion quality samples: " << report.num_samples << "\n";
    std::cout << "Expansion p95 abs error: " << report.p95_abs_error << "\n";
    std::cout << "Sign mismatch rate: " << report.sign_mismatch_rate << "\n";
    std::cout << "Near-surface sign mismatch rate: "
              << report.near_surface_sign_mismatch_rate << "\n";
    std::cout << "Status: ok\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "capability walkthrough failed: " << exc.what() << "\n";
    return 1;
  }
}

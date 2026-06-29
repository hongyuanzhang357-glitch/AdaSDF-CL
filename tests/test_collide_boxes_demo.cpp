#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

bool contactsFinite(const adasdf::CollisionResult& result) {
  for (const adasdf::Contact& contact : result.contacts()) {
    if (!contact.point.allFinite() || !contact.normal.allFinite() ||
        !std::isfinite(contact.penetration_depth) ||
        contact.penetration_depth < 0.0) {
      return false;
    }
  }
  return true;
}

}  // namespace

int main() {
  try {
    adasdf::DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = true;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

    adasdf::CollisionObject a(build.model);
    adasdf::CollisionObject b(build.model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 2;
    request.enable_gradient = true;

    adasdf::CollisionResult result;
    if (!adasdf::collide(a, b, request, result)) {
      std::cerr << "demo adaptive boxes should collide\n";
      return 1;
    }
    if (result.contacts().empty() ||
        result.contacts().size() > static_cast<std::size_t>(request.max_contacts) ||
        !contactsFinite(result)) {
      std::cerr << "demo adaptive contacts are invalid or max_contacts failed\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_collide_boxes_demo failed: "
              << exc.what() << "\n";
    return 1;
  }
}

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
    auto model = adasdf::AnalyticSDFModel::createBox();

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 3;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = adasdf::QueryMode::Balanced;

    adasdf::CollisionObject a(model);
    adasdf::CollisionObject b(model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionResult collision;
    if (!adasdf::collide(a, b, request, collision)) {
      std::cerr << "overlapping analytic boxes should collide\n";
      return 1;
    }
    if (collision.contacts().empty() ||
        collision.contacts().size() > static_cast<std::size_t>(request.max_contacts) ||
        !contactsFinite(collision)) {
      std::cerr << "analytic collision contacts are invalid\n";
      return 1;
    }

    adasdf::DistanceRequest distance_request;
    adasdf::DistanceResult distance_result;
    const double d = adasdf::distance(a, b, distance_request, distance_result);
    if (!std::isfinite(d) || !distance_result.hasResult()) {
      std::cerr << "analytic distance result is invalid\n";
      return 1;
    }

    adasdf::CollisionObject separated_a(model);
    adasdf::CollisionObject separated_b(model);
    separated_b.setTransform(adasdf::Transform::fromTranslation({2.0, 0.0, 0.0}));
    adasdf::CollisionResult separated;
    if (adasdf::collide(separated_a, separated_b, request, separated)) {
      std::cerr << "separated analytic boxes should not collide\n";
      return 1;
    }
    if (!std::isfinite(separated.minimumDistance()) ||
        separated.minimumDistance() <= 0.0) {
      std::cerr << "separated analytic box distance should be positive\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_core_free_demo_collision failed: " << exc.what() << "\n";
    return 1;
  }
}

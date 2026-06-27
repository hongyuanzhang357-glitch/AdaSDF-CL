#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

double norm(const adasdf::Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

bool sameContacts(
    const std::vector<adasdf::Contact>& a,
    const std::vector<adasdf::Contact>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i].point.x - b[i].point.x) > 1.0e-12 ||
        std::abs(a[i].point.y - b[i].point.y) > 1.0e-12 ||
        std::abs(a[i].point.z - b[i].point.z) > 1.0e-12 ||
        std::abs(a[i].penetration_depth - b[i].penetration_depth) > 1.0e-12) {
      return false;
    }
  }
  return true;
}

}  // namespace

int main() {
  std::vector<adasdf::RawContactCandidate> raw;
  for (int i = 0; i < 20; ++i) {
    adasdf::RawContactCandidate contact;
    contact.point_world = {0.001 * static_cast<double>(i), 0.0, 0.0};
    contact.normal_world = {1.0, 0.0, 0.0};
    contact.signed_distance = -0.01 - 0.001 * static_cast<double>(i);
    contact.penetration_depth = -contact.signed_distance;
    contact.source_object_id = 1;
    contact.target_object_id = 2;
    raw.push_back(contact);
  }

  adasdf::ContactReductionOptions options;
  options.max_contacts = 4;
  options.position_merge_tolerance = 1.0e-4;
  options.normal_merge_cosine = 0.95;

  adasdf::CollisionObject obj_a;
  adasdf::CollisionObject obj_b;
  obj_a.setObjectId(1);
  obj_b.setObjectId(2);

  const auto reduced_a =
      adasdf::ContactReducer::reduce(raw, obj_a, obj_b, options);
  const auto reduced_b =
      adasdf::ContactReducer::reduce(raw, obj_a, obj_b, options);

  if (reduced_a.empty() ||
      reduced_a.size() > static_cast<std::size_t>(options.max_contacts)) {
    std::cerr << "contact reducer did not respect max_contacts\n";
    return 1;
  }
  if (!sameContacts(reduced_a, reduced_b)) {
    std::cerr << "contact reducer is not deterministic\n";
    return 1;
  }
  for (const adasdf::Contact& contact : reduced_a) {
    if (contact.penetration_depth < 0.0 ||
        !std::isfinite(contact.penetration_depth)) {
      std::cerr << "reduced contact has invalid penetration depth\n";
      return 1;
    }
    const double n = norm(contact.normal);
    if (std::abs(n - 1.0) > 1.0e-9) {
      std::cerr << "reduced contact normal is not normalized\n";
      return 1;
    }
  }

  return 0;
}

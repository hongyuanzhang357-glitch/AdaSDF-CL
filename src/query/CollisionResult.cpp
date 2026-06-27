#include "adasdf/query/CollisionResult.h"

namespace adasdf {

void CollisionResult::clear() {
  is_colliding_ = false;
  minimum_distance_ = 0.0;
  contacts_.clear();
  timing_ = {};
  sdf_query_count_ = 0;
  candidate_point_count_ = 0;
  raw_contact_count_ = 0;
  reduced_contact_count_ = 0;
  backend_info_.clear();
  method_info_.clear();
}

void CollisionResult::addContact(const Contact& contact) {
  contacts_.push_back(contact);
  is_colliding_ = true;
  if (contacts_.size() == 1 || contact.signed_distance < minimum_distance_) {
    minimum_distance_ = contact.signed_distance;
  }
}

}  // namespace adasdf

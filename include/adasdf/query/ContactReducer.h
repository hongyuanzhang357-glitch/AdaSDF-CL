#pragma once

#include <vector>

#include "adasdf/query/ContactGenerator.h"

namespace adasdf {

struct ContactReductionOptions {
  int max_contacts = 32;
  Scalar position_merge_tolerance = 1.0e-4;
  Scalar normal_merge_cosine = 0.95;
  bool prefer_deep_contacts = true;
};

class ContactReducer {
 public:
  static std::vector<Contact> reduce(
      const std::vector<RawContactCandidate>& raw_contacts,
      const CollisionObject& obj_a,
      const CollisionObject& obj_b,
      const ContactReductionOptions& options);
};

}  // namespace adasdf

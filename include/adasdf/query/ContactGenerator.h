#pragma once

#include <vector>

#include "adasdf/query/CpuNarrowPhase.h"

namespace adasdf {

struct RawContactCandidate {
  Vector3 point_world;
  Vector3 normal_world;
  Scalar signed_distance = 0.0;
  Scalar penetration_depth = 0.0;
  int source_object_id = -1;
  int target_object_id = -1;
};

class ContactGenerator {
 public:
  static std::vector<RawContactCandidate> generateSymmetricContacts(
      const CollisionObject& obj_a,
      const CollisionObject& obj_b,
      const CollisionRequest& request,
      NarrowPhaseStats& stats);
};

}  // namespace adasdf

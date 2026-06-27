#pragma once

#include <cstddef>
#include <string>

#include "adasdf/query/CollisionObject.h"
#include "adasdf/query/CollisionRequest.h"
#include "adasdf/query/CollisionResult.h"
#include "adasdf/query/DistanceRequest.h"
#include "adasdf/query/DistanceResult.h"

namespace adasdf {

struct NarrowPhaseStats {
  std::size_t num_candidate_points = 0;
  std::size_t num_sdf_queries = 0;
  std::size_t num_raw_contacts = 0;
  std::size_t num_reduced_contacts = 0;
  bool used_symmetric_query = false;
  std::string method;
};

class CpuNarrowPhase {
 public:
  static bool collide(
      const CollisionObject& obj_a,
      const CollisionObject& obj_b,
      const CollisionRequest& request,
      CollisionResult& result);

  static Scalar distance(
      const CollisionObject& obj_a,
      const CollisionObject& obj_b,
      const DistanceRequest& request,
      DistanceResult& result);
};

}  // namespace adasdf

#pragma once

#include "adasdf/query/CollisionObject.h"
#include "adasdf/query/CollisionRequest.h"
#include "adasdf/query/CollisionResult.h"
#include "adasdf/query/DistanceRequest.h"
#include "adasdf/query/DistanceResult.h"

namespace adasdf {

class ExistingPairCollisionBridge {
 public:
  static bool isAvailable();

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

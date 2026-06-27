#include "adasdf/query/ExistingPairCollisionBridge.h"

namespace adasdf {

bool ExistingPairCollisionBridge::isAvailable() {
  return false;
}

bool ExistingPairCollisionBridge::collide(
    const CollisionObject&,
    const CollisionObject&,
    const CollisionRequest&,
    CollisionResult&) {
  return false;
}

Scalar ExistingPairCollisionBridge::distance(
    const CollisionObject&,
    const CollisionObject&,
    const DistanceRequest&,
    DistanceResult&) {
  return 0.0;
}

}  // namespace adasdf

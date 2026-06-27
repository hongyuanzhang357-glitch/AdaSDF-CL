#include "adasdf/adapters/FCLAdapter.h"

namespace adasdf {

bool FCLAdapter::collide(
    const FCLStyleCollisionObject& object_a,
    const FCLStyleCollisionObject& object_b,
    const CollisionRequest& request,
    CollisionResult& result) {
  return adasdf::collide(object_a.object(), object_b.object(), request, result);
}

Scalar FCLAdapter::distance(
    const FCLStyleCollisionObject& object_a,
    const FCLStyleCollisionObject& object_b,
    const DistanceRequest& request,
    DistanceResult& result) {
  return adasdf::distance(object_a.object(), object_b.object(), request, result);
}

}  // namespace adasdf

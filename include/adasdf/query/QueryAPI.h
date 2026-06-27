#pragma once

#include <vector>

#include "adasdf/query/CollisionObject.h"
#include "adasdf/query/CollisionRequest.h"
#include "adasdf/query/CollisionResult.h"
#include "adasdf/query/DistanceRequest.h"
#include "adasdf/query/DistanceResult.h"

namespace adasdf {

struct BatchedCollisionRequest {
  CollisionRequest request;
  std::vector<const CollisionObject*> objects_a;
  std::vector<const CollisionObject*> objects_b;
};

struct BatchedCollisionResult {
  std::vector<CollisionResult> results;
};

// TODO: route Fast mode to all-points SDF query, Balanced mode to
// sdf::PairCollisionSDF, and Accurate mode to a refined contact search.
bool collide(
    const CollisionObject& object_a,
    const CollisionObject& object_b,
    const CollisionRequest& request,
    CollisionResult& result);

// TODO: connect to nearest-distance logic once the CPU SDF query API is stable.
Scalar distance(
    const CollisionObject& object_a,
    const CollisionObject& object_b,
    const DistanceRequest& request,
    DistanceResult& result);

// TODO: route CUDA batches to sdf::CudaAllPointsSDFContact when available.
BatchedCollisionResult collideBatch(const BatchedCollisionRequest& request);

}  // namespace adasdf

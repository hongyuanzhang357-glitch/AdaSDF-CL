#include "adasdf/query/QueryAPI.h"

#include <stdexcept>

#include "adasdf/query/CpuNarrowPhase.h"
#include "adasdf/query/ExistingPairCollisionBridge.h"

namespace adasdf {
namespace {

void requireCPUBackend(BackendType backend) {
  if (backend == BackendType::CUDA) {
    throw std::runtime_error(
        "AdaSDF-CL v0.5 pair query supports CPU narrow-phase only; CUDA "
        "batched pair query is still a TODO.");
  }
}

}  // namespace

bool collide(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const CollisionRequest& request,
    CollisionResult& result) {
  requireCPUBackend(request.backend);

  if (ExistingPairCollisionBridge::isAvailable()) {
    return ExistingPairCollisionBridge::collide(obj_a, obj_b, request, result);
  }

  return CpuNarrowPhase::collide(obj_a, obj_b, request, result);
}

Scalar distance(
    const CollisionObject& obj_a,
    const CollisionObject& obj_b,
    const DistanceRequest& request,
    DistanceResult& result) {
  requireCPUBackend(request.backend);

  if (ExistingPairCollisionBridge::isAvailable()) {
    return ExistingPairCollisionBridge::distance(obj_a, obj_b, request, result);
  }

  return CpuNarrowPhase::distance(obj_a, obj_b, request, result);
}

BatchedCollisionResult collideBatch(const BatchedCollisionRequest& request) {
  if (request.objects_a.size() != request.objects_b.size()) {
    throw std::runtime_error("collideBatch requires objects_a and objects_b to match.");
  }

  BatchedCollisionResult batch_result;
  batch_result.results.resize(request.objects_a.size());
  for (std::size_t i = 0; i < request.objects_a.size(); ++i) {
    if (!request.objects_a[i] || !request.objects_b[i]) {
      continue;
    }
    collide(
        *request.objects_a[i],
        *request.objects_b[i],
        request.request,
        batch_result.results[i]);
  }
  return batch_result;
}

}  // namespace adasdf

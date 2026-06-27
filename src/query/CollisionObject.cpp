#include "adasdf/query/CollisionObject.h"

#include <algorithm>
#include <array>
#include <atomic>

namespace adasdf {
namespace {

ObjectId nextObjectId() {
  static std::atomic<ObjectId> next_id{1};
  return next_id.fetch_add(1);
}

void expand(AABB& bounds, const Vector3& point) {
  if (!bounds.valid) {
    bounds.min = point;
    bounds.max = point;
    bounds.valid = true;
    return;
  }

  bounds.min.x = std::min(bounds.min.x, point.x);
  bounds.min.y = std::min(bounds.min.y, point.y);
  bounds.min.z = std::min(bounds.min.z, point.z);
  bounds.max.x = std::max(bounds.max.x, point.x);
  bounds.max.y = std::max(bounds.max.y, point.y);
  bounds.max.z = std::max(bounds.max.z, point.z);
}

}  // namespace

CollisionObject::CollisionObject(std::shared_ptr<SDFModel> model)
    : model_(std::move(model)), object_id_(nextObjectId()) {}

AABB CollisionObject::getAABB() const {
  if (!model_) {
    return {};
  }

  const AABB local = model_->boundingBox();
  if (!local.valid) {
    return {};
  }

  const std::array<Vector3, 8> corners = {
      Vector3{local.min.x, local.min.y, local.min.z},
      Vector3{local.max.x, local.min.y, local.min.z},
      Vector3{local.min.x, local.max.y, local.min.z},
      Vector3{local.max.x, local.max.y, local.min.z},
      Vector3{local.min.x, local.min.y, local.max.z},
      Vector3{local.max.x, local.min.y, local.max.z},
      Vector3{local.min.x, local.max.y, local.max.z},
      Vector3{local.max.x, local.max.y, local.max.z}};

  AABB world;
  for (const Vector3& corner : corners) {
    expand(world, transform_.applyPoint(corner));
  }
  return world;
}

}  // namespace adasdf

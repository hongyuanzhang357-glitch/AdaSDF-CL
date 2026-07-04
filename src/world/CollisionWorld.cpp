#include "adasdf/world/CollisionWorld.h"

#include <algorithm>

namespace adasdf {
namespace {

AABB objectLocalAABB(const WorldObject& object) {
  if (object.model && object.model->boundingBox().valid) {
    return object.model->boundingBox();
  }
  if (object.has_samples) {
    return object.samples.boundingBox();
  }
  return {};
}

}  // namespace

int CollisionWorld::addObject(WorldObject object) {
  if (object.object_id < 0 || containsObjectId(object.object_id)) {
    object.object_id = nextObjectId();
  }
  object.name = worldObjectDisplayName(object);
  object.local_aabb = objectLocalAABB(object);
  object.world_aabb = object.transform.applyAABB(object.local_aabb);
  const int object_id = object.object_id;
  objects_.push_back(std::move(object));
  std::sort(
      objects_.begin(),
      objects_.end(),
      [](const WorldObject& a, const WorldObject& b) {
        return a.object_id < b.object_id;
      });
  return object_id;
}

bool CollisionWorld::removeObject(int object_id) {
  const auto old_size = objects_.size();
  objects_.erase(
      std::remove_if(
          objects_.begin(),
          objects_.end(),
          [&](const WorldObject& object) {
            return object.object_id == object_id;
          }),
      objects_.end());
  return objects_.size() != old_size;
}

bool CollisionWorld::updateTransform(
    int object_id,
    const WorldTransform& transform) {
  WorldObject* object = findObject(object_id);
  if (!object) {
    return false;
  }
  object->transform = transform;
  object->world_aabb = object->transform.applyAABB(object->local_aabb);
  return true;
}

bool CollisionWorld::updateEnabled(int object_id, bool enabled) {
  WorldObject* object = findObject(object_id);
  if (!object) {
    return false;
  }
  object->enabled = enabled;
  return true;
}

bool CollisionWorld::refreshObjectAABB(int object_id) {
  WorldObject* object = findObject(object_id);
  if (!object) {
    return false;
  }
  object->local_aabb = objectLocalAABB(*object);
  object->world_aabb = object->transform.applyAABB(object->local_aabb);
  return true;
}

void CollisionWorld::refreshAABBs() {
  for (WorldObject& object : objects_) {
    object.local_aabb = objectLocalAABB(object);
    object.world_aabb = object.transform.applyAABB(object.local_aabb);
  }
}

void CollisionWorld::clear() {
  objects_.clear();
}

const WorldObject* CollisionWorld::findObject(int object_id) const {
  for (const WorldObject& object : objects_) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

WorldObject* CollisionWorld::findObject(int object_id) {
  for (WorldObject& object : objects_) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

int CollisionWorld::nextObjectId() const {
  int next = 0;
  for (const WorldObject& object : objects_) {
    next = std::max(next, object.object_id + 1);
  }
  return next;
}

bool CollisionWorld::containsObjectId(int object_id) const {
  return findObject(object_id) != nullptr;
}

}  // namespace adasdf

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/world/WorldObject.h"

namespace adasdf {

class CollisionWorld {
 public:
  int addObject(WorldObject object);
  bool removeObject(int object_id);
  bool updateTransform(int object_id, const WorldTransform& transform);
  bool updateEnabled(int object_id, bool enabled);
  bool refreshObjectAABB(int object_id);
  void refreshAABBs();
  void clear();

  std::size_t objectCount() const {
    return objects_.size();
  }

  bool empty() const {
    return objects_.empty();
  }

  const std::vector<WorldObject>& objects() const {
    return objects_;
  }

  std::vector<WorldObject>& objects() {
    return objects_;
  }

  const WorldObject* findObject(int object_id) const;
  WorldObject* findObject(int object_id);

 private:
  int nextObjectId() const;
  bool containsObjectId(int object_id) const;

  std::vector<WorldObject> objects_;
};

}  // namespace adasdf

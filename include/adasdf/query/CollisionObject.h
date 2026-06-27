#pragma once

#include <memory>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class CollisionObject {
 public:
  CollisionObject() = default;
  explicit CollisionObject(std::shared_ptr<SDFModel> model);

  void setTransform(const Transform& transform) {
    transform_ = transform;
  }

  const Transform& getTransform() const {
    return transform_;
  }

  const Transform& transform() const {
    return transform_;
  }

  AABB getAABB() const;

  const std::shared_ptr<SDFModel>& getModel() const {
    return model_;
  }

  void setObjectId(ObjectId id) {
    object_id_ = id;
  }

  ObjectId objectId() const {
    return object_id_;
  }

  ObjectId id() const {
    return object_id_;
  }

  void setUserData(void* user_data) {
    user_data_ = user_data;
  }

  void* userData() const {
    return user_data_;
  }

 private:
  std::shared_ptr<SDFModel> model_;
  Transform transform_;
  ObjectId object_id_ = -1;
  void* user_data_ = nullptr;
};

}  // namespace adasdf

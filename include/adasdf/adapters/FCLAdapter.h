#pragma once

#include "adasdf/query/QueryAPI.h"

namespace adasdf {

struct FCLAdapterOptions {
  bool prefer_sdf_for_contact = true;
  bool allow_triangle_fallback = false;
  BackendType backend = BackendType::Auto;
};

class FCLStyleCollisionObject {
 public:
  explicit FCLStyleCollisionObject(CollisionObject object)
      : object_(object) {}

  CollisionObject& object() {
    return object_;
  }

  const CollisionObject& object() const {
    return object_;
  }

 private:
  CollisionObject object_;
};

class FCLAdapter {
 public:
  // This header intentionally does not include or link FCL. It is a migration
  // surface for projects that already organize queries around FCL-like objects.
  static bool collide(
      const FCLStyleCollisionObject& object_a,
      const FCLStyleCollisionObject& object_b,
      const CollisionRequest& request,
      CollisionResult& result);

  static Scalar distance(
      const FCLStyleCollisionObject& object_a,
      const FCLStyleCollisionObject& object_b,
      const DistanceRequest& request,
      DistanceResult& result);
};

}  // namespace adasdf

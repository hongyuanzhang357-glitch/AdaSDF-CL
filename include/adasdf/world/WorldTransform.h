#pragma once

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct WorldQuaternion {
  double w = 1.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

class WorldTransform {
 public:
  static WorldTransform Identity();
  static WorldTransform fromTranslation(const Vector3& translation);
  static WorldTransform fromQuaternionTranslation(
      WorldQuaternion quaternion,
      const Vector3& translation);

  const Vector3& translation() const {
    return translation_;
  }

  const WorldQuaternion& quaternion() const {
    return quaternion_;
  }

  Transform toTransform() const;
  Transform inverseTransform() const;

  Vector3 applyPoint(const Vector3& local_point) const;
  Vector3 applyVector(const Vector3& local_vector) const;
  Vector3 inverseApplyPoint(const Vector3& world_point) const;
  Vector3 inverseApplyVector(const Vector3& world_vector) const;
  AABB applyAABB(const AABB& local_aabb) const;

 private:
  Vector3 translation_;
  WorldQuaternion quaternion_;
};

WorldQuaternion normalizedQuaternionOrIdentity(WorldQuaternion quaternion);
Matrix3 rotationMatrixFromQuaternion(WorldQuaternion quaternion);

}  // namespace adasdf

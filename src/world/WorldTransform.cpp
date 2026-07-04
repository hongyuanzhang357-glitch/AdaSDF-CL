#include "adasdf/world/WorldTransform.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

bool finiteQuaternion(const WorldQuaternion& q) {
  return std::isfinite(q.w) && std::isfinite(q.x) && std::isfinite(q.y) &&
      std::isfinite(q.z);
}

void expand(AABB* box, const Vector3& point) {
  if (!box->valid) {
    box->min = point;
    box->max = point;
    box->valid = point.allFinite();
    return;
  }
  box->min.x = std::min(box->min.x, point.x);
  box->min.y = std::min(box->min.y, point.y);
  box->min.z = std::min(box->min.z, point.z);
  box->max.x = std::max(box->max.x, point.x);
  box->max.y = std::max(box->max.y, point.y);
  box->max.z = std::max(box->max.z, point.z);
}

}  // namespace

WorldQuaternion normalizedQuaternionOrIdentity(WorldQuaternion quaternion) {
  if (!finiteQuaternion(quaternion)) {
    return {};
  }
  const double norm = std::sqrt(
      quaternion.w * quaternion.w + quaternion.x * quaternion.x +
      quaternion.y * quaternion.y + quaternion.z * quaternion.z);
  if (!(norm > std::numeric_limits<double>::epsilon())) {
    return {};
  }
  quaternion.w /= norm;
  quaternion.x /= norm;
  quaternion.y /= norm;
  quaternion.z /= norm;
  return quaternion;
}

Matrix3 rotationMatrixFromQuaternion(WorldQuaternion quaternion) {
  quaternion = normalizedQuaternionOrIdentity(quaternion);
  const double w = quaternion.w;
  const double x = quaternion.x;
  const double y = quaternion.y;
  const double z = quaternion.z;
  Matrix3 matrix;
  matrix.values = {
      1.0 - 2.0 * (y * y + z * z),
      2.0 * (x * y - z * w),
      2.0 * (x * z + y * w),
      2.0 * (x * y + z * w),
      1.0 - 2.0 * (x * x + z * z),
      2.0 * (y * z - x * w),
      2.0 * (x * z - y * w),
      2.0 * (y * z + x * w),
      1.0 - 2.0 * (x * x + y * y)};
  return matrix;
}

WorldTransform WorldTransform::Identity() {
  return {};
}

WorldTransform WorldTransform::fromTranslation(const Vector3& translation) {
  WorldTransform transform;
  transform.translation_ = translation;
  return transform;
}

WorldTransform WorldTransform::fromQuaternionTranslation(
    WorldQuaternion quaternion,
    const Vector3& translation) {
  WorldTransform transform;
  transform.translation_ = translation;
  transform.quaternion_ = normalizedQuaternionOrIdentity(quaternion);
  return transform;
}

Transform WorldTransform::toTransform() const {
  return Transform::fromRotationTranslation(
      rotationMatrixFromQuaternion(quaternion_),
      translation_);
}

Transform WorldTransform::inverseTransform() const {
  return toTransform().inverse();
}

Vector3 WorldTransform::applyPoint(const Vector3& local_point) const {
  return toTransform().applyPoint(local_point);
}

Vector3 WorldTransform::applyVector(const Vector3& local_vector) const {
  return toTransform().applyVector(local_vector);
}

Vector3 WorldTransform::inverseApplyPoint(const Vector3& world_point) const {
  return toTransform().inverseApplyPoint(world_point);
}

Vector3 WorldTransform::inverseApplyVector(const Vector3& world_vector) const {
  return inverseTransform().applyVector(world_vector);
}

AABB WorldTransform::applyAABB(const AABB& local_aabb) const {
  if (!local_aabb.valid) {
    return {};
  }
  AABB result;
  for (int ix = 0; ix < 2; ++ix) {
    for (int iy = 0; iy < 2; ++iy) {
      for (int iz = 0; iz < 2; ++iz) {
        expand(
            &result,
            applyPoint(Vector3{
                ix == 0 ? local_aabb.min.x : local_aabb.max.x,
                iy == 0 ? local_aabb.min.y : local_aabb.max.y,
                iz == 0 ? local_aabb.min.z : local_aabb.max.z}));
      }
    }
  }
  return result;
}

}  // namespace adasdf

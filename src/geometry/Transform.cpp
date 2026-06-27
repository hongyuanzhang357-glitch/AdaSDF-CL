#include "adasdf/geometry/Transform.h"

namespace adasdf {

Vector3 Transform::applyPoint(const Vector3& point_local) const {
  const auto& r = rotation_.values;
  return {
      r[0] * point_local.x + r[1] * point_local.y + r[2] * point_local.z +
          translation_.x,
      r[3] * point_local.x + r[4] * point_local.y + r[5] * point_local.z +
          translation_.y,
      r[6] * point_local.x + r[7] * point_local.y + r[8] * point_local.z +
          translation_.z};
}

Vector3 Transform::applyVector(const Vector3& vector_local) const {
  const auto& r = rotation_.values;
  return {
      r[0] * vector_local.x + r[1] * vector_local.y + r[2] * vector_local.z,
      r[3] * vector_local.x + r[4] * vector_local.y + r[5] * vector_local.z,
      r[6] * vector_local.x + r[7] * vector_local.y + r[8] * vector_local.z};
}

Vector3 Transform::inverseApplyPoint(const Vector3& point_world) const {
  const Vector3 shifted = point_world - translation_;
  const auto& r = rotation_.values;
  return {
      r[0] * shifted.x + r[3] * shifted.y + r[6] * shifted.z,
      r[1] * shifted.x + r[4] * shifted.y + r[7] * shifted.z,
      r[2] * shifted.x + r[5] * shifted.y + r[8] * shifted.z};
}

Vector3 Transform::apply(const Vector3& point) const {
  return applyPoint(point);
}

Transform Transform::inverse() const {
  Transform result;
  const auto& r = rotation_.values;
  auto& ri = result.rotation_.values;

  ri = {r[0], r[3], r[6],
        r[1], r[4], r[7],
        r[2], r[5], r[8]};

  const Vector3 t = translation_;
  result.translation_ = {
      -(ri[0] * t.x + ri[1] * t.y + ri[2] * t.z),
      -(ri[3] * t.x + ri[4] * t.y + ri[5] * t.z),
      -(ri[6] * t.x + ri[7] * t.y + ri[8] * t.z)};
  return result;
}

}  // namespace adasdf

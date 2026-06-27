#pragma once

#include <array>
#include <cmath>
#include <ostream>
#include <vector>

#include "adasdf/config.h"

namespace adasdf {

struct Vector3 {
  Scalar x = 0.0;
  Scalar y = 0.0;
  Scalar z = 0.0;

  static Vector3 Zero() {
    return {};
  }

  const Vector3& transpose() const {
    return *this;
  }

  bool allFinite() const {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
  }
};

inline Vector3 operator+(const Vector3& a, const Vector3& b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vector3 operator-(const Vector3& a, const Vector3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 operator*(Scalar s, const Vector3& v) {
  return {s * v.x, s * v.y, s * v.z};
}

inline Vector3 operator*(const Vector3& v, Scalar s) {
  return s * v;
}

inline Vector3 operator/(const Vector3& v, Scalar s) {
  return {v.x / s, v.y / s, v.z / s};
}

inline std::ostream& operator<<(std::ostream& os, const Vector3& v) {
  os << v.x << " " << v.y << " " << v.z;
  return os;
}

struct Matrix3 {
  std::array<Scalar, 9> values = {1.0, 0.0, 0.0,
                                  0.0, 1.0, 0.0,
                                  0.0, 0.0, 1.0};

  static Matrix3 Identity() {
    return {};
  }

  Scalar operator()(std::size_t row, std::size_t col) const {
    return values[row * 3 + col];
  }
};

struct MatrixX {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::vector<Scalar> values;
};

struct AABB {
  Vector3 min;
  Vector3 max;

  bool valid = false;
};

class Transform {
 public:
  static Transform Identity() {
    return {};
  }

  static Transform fromTranslation(const Vector3& translation) {
    Transform transform;
    transform.translation_ = translation;
    return transform;
  }

  static Transform fromRotationTranslation(
      const Matrix3& rotation,
      const Vector3& translation) {
    Transform transform;
    transform.rotation_ = rotation;
    transform.translation_ = translation;
    return transform;
  }

  const Matrix3& rotation() const {
    return rotation_;
  }

  const Vector3& translation() const {
    return translation_;
  }

  void setRotation(const Matrix3& rotation) {
    rotation_ = rotation;
  }

  void setTranslation(const Vector3& translation) {
    translation_ = translation;
  }

  Vector3 applyPoint(const Vector3& point_local) const;
  Vector3 applyVector(const Vector3& vector_local) const;
  Vector3 inverseApplyPoint(const Vector3& point_world) const;
  Vector3 apply(const Vector3& point) const;
  Transform inverse() const;

 private:
  Matrix3 rotation_;
  Vector3 translation_;
};

}  // namespace adasdf

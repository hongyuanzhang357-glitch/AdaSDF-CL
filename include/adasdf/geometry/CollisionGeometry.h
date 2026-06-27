#pragma once

#include <string>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

enum class GeometryKind {
  Unknown,
  Mesh,
  SDFModel
};

class CollisionGeometry {
 public:
  virtual ~CollisionGeometry() = default;

  virtual GeometryKind kind() const = 0;
  virtual AABB localAABB() const = 0;
  virtual std::string debugName() const = 0;
};

}  // namespace adasdf

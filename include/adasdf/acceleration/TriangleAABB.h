#pragma once

#include <cstddef>

#include "adasdf/geometry/Transform.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct TriangleAABB {
  Vector3 min;
  Vector3 max;
  bool valid = false;
};

TriangleAABB makeEmptyTriangleAABB();
TriangleAABB makeTriangleAABB(
    const Vector3& a,
    const Vector3& b,
    const Vector3& c);
TriangleAABB makeMeshTriangleAABB(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle);
TriangleAABB mergeTriangleAABB(
    const TriangleAABB& lhs,
    const TriangleAABB& rhs);

bool isValid(const TriangleAABB& box);
Vector3 center(const TriangleAABB& box);
Vector3 extent(const TriangleAABB& box);
double surfaceArea(const TriangleAABB& box);
double squaredDistanceToPoint(const TriangleAABB& box, const Vector3& point);
double axisValue(const Vector3& value, int axis);
void expandToInclude(TriangleAABB* box, const Vector3& point);
void expandToInclude(TriangleAABB* box, const TriangleAABB& other);

}  // namespace adasdf

#include "adasdf/acceleration/TriangleAABB.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {

TriangleAABB makeEmptyTriangleAABB() {
  TriangleAABB box;
  const double inf = std::numeric_limits<double>::infinity();
  box.min = {inf, inf, inf};
  box.max = {-inf, -inf, -inf};
  box.valid = false;
  return box;
}

void expandToInclude(TriangleAABB* box, const Vector3& point) {
  if (box == nullptr || !point.allFinite()) {
    return;
  }
  if (!box->valid) {
    box->min = point;
    box->max = point;
    box->valid = true;
    return;
  }
  box->min.x = std::min(box->min.x, point.x);
  box->min.y = std::min(box->min.y, point.y);
  box->min.z = std::min(box->min.z, point.z);
  box->max.x = std::max(box->max.x, point.x);
  box->max.y = std::max(box->max.y, point.y);
  box->max.z = std::max(box->max.z, point.z);
}

void expandToInclude(TriangleAABB* box, const TriangleAABB& other) {
  if (box == nullptr || !isValid(other)) {
    return;
  }
  expandToInclude(box, other.min);
  expandToInclude(box, other.max);
}

TriangleAABB makeTriangleAABB(
    const Vector3& a,
    const Vector3& b,
    const Vector3& c) {
  TriangleAABB box = makeEmptyTriangleAABB();
  expandToInclude(&box, a);
  expandToInclude(&box, b);
  expandToInclude(&box, c);
  return box;
}

TriangleAABB makeMeshTriangleAABB(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle) {
  const auto valid_index = [&](int index) {
    return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
  };
  if (!valid_index(triangle.v0) || !valid_index(triangle.v1) ||
      !valid_index(triangle.v2)) {
    return makeEmptyTriangleAABB();
  }
  return makeTriangleAABB(
      toVector3(mesh.vertices[triangle.v0]),
      toVector3(mesh.vertices[triangle.v1]),
      toVector3(mesh.vertices[triangle.v2]));
}

TriangleAABB mergeTriangleAABB(
    const TriangleAABB& lhs,
    const TriangleAABB& rhs) {
  TriangleAABB out = makeEmptyTriangleAABB();
  expandToInclude(&out, lhs);
  expandToInclude(&out, rhs);
  return out;
}

bool isValid(const TriangleAABB& box) {
  return box.valid && box.min.allFinite() && box.max.allFinite() &&
         box.max.x >= box.min.x && box.max.y >= box.min.y &&
         box.max.z >= box.min.z;
}

Vector3 center(const TriangleAABB& box) {
  return 0.5 * (box.min + box.max);
}

Vector3 extent(const TriangleAABB& box) {
  if (!isValid(box)) {
    return {};
  }
  return box.max - box.min;
}

double surfaceArea(const TriangleAABB& box) {
  if (!isValid(box)) {
    return 0.0;
  }
  const Vector3 e = extent(box);
  return 2.0 * (e.x * e.y + e.x * e.z + e.y * e.z);
}

double squaredDistanceToPoint(const TriangleAABB& box, const Vector3& point) {
  if (!isValid(box) || !point.allFinite()) {
    return std::numeric_limits<double>::infinity();
  }
  double d2 = 0.0;
  const double p[3] = {point.x, point.y, point.z};
  const double lo[3] = {box.min.x, box.min.y, box.min.z};
  const double hi[3] = {box.max.x, box.max.y, box.max.z};
  for (int axis = 0; axis < 3; ++axis) {
    double delta = 0.0;
    if (p[axis] < lo[axis]) {
      delta = lo[axis] - p[axis];
    } else if (p[axis] > hi[axis]) {
      delta = p[axis] - hi[axis];
    }
    d2 += delta * delta;
  }
  return d2;
}

double axisValue(const Vector3& value, int axis) {
  if (axis == 0) {
    return value.x;
  }
  if (axis == 1) {
    return value.y;
  }
  return value.z;
}

}  // namespace adasdf

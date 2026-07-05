#include "adasdf/geometry/BoxTriangleDistance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "adasdf/acceleration/TriangleAABB.h"
#include "adasdf/mesh/TriangleDistance.h"

namespace adasdf {
namespace {

bool validBox(const AABB& box) {
  return box.valid && box.min.allFinite() && box.max.allFinite() &&
         box.max.x >= box.min.x && box.max.y >= box.min.y &&
         box.max.z >= box.min.z;
}

double squaredDistanceBetweenAABBs(const AABB& lhs, const TriangleAABB& rhs) {
  if (!validBox(lhs) || !isValid(rhs)) {
    return std::numeric_limits<double>::infinity();
  }
  double d2 = 0.0;
  const double lo_a[3] = {lhs.min.x, lhs.min.y, lhs.min.z};
  const double hi_a[3] = {lhs.max.x, lhs.max.y, lhs.max.z};
  const double lo_b[3] = {rhs.min.x, rhs.min.y, rhs.min.z};
  const double hi_b[3] = {rhs.max.x, rhs.max.y, rhs.max.z};
  for (int axis = 0; axis < 3; ++axis) {
    double delta = 0.0;
    if (hi_a[axis] < lo_b[axis]) {
      delta = lo_b[axis] - hi_a[axis];
    } else if (hi_b[axis] < lo_a[axis]) {
      delta = lo_a[axis] - hi_b[axis];
    }
    d2 += delta * delta;
  }
  return d2;
}

bool containsPoint(const AABB& box, const Vector3& point) {
  return validBox(box) && point.allFinite() &&
         point.x >= box.min.x && point.x <= box.max.x &&
         point.y >= box.min.y && point.y <= box.max.y &&
         point.z >= box.min.z && point.z <= box.max.z;
}

double squaredDistancePointBox(const Vector3& point, const AABB& box) {
  if (!validBox(box) || !point.allFinite()) {
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

Vector3 boxCenter(const AABB& box) {
  return 0.5 * (box.min + box.max);
}

std::array<Vector3, 8> boxCorners(const AABB& box) {
  return {{
      {box.min.x, box.min.y, box.min.z},
      {box.max.x, box.min.y, box.min.z},
      {box.min.x, box.max.y, box.min.z},
      {box.max.x, box.max.y, box.min.z},
      {box.min.x, box.min.y, box.max.z},
      {box.max.x, box.min.y, box.max.z},
      {box.min.x, box.max.y, box.max.z},
      {box.max.x, box.max.y, box.max.z},
  }};
}

double safeSqrt(double value) {
  if (!std::isfinite(value)) {
    return std::numeric_limits<double>::infinity();
  }
  return std::sqrt(std::max(0.0, value));
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

}  // namespace

BoxTriangleDistanceResult BoxTriangleDistance::estimate(
    const AABB& box,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c) {
  BoxTriangleDistanceResult result;
  if (!validBox(box) || !a.allFinite() || !b.allFinite() || !c.allFinite()) {
    result.lower_bound_distance = std::numeric_limits<double>::infinity();
    result.approximate_distance = std::numeric_limits<double>::infinity();
    return result;
  }

  const TriangleAABB tri_box = makeTriangleAABB(a, b, c);
  result.lower_bound_distance =
      safeSqrt(squaredDistanceBetweenAABBs(box, tri_box));

  double best2 = pointTriangleSquaredDistance(boxCenter(box), a, b, c);
  for (const Vector3& corner : boxCorners(box)) {
    best2 = std::min(best2, pointTriangleSquaredDistance(corner, a, b, c));
  }
  best2 = std::min(best2, squaredDistancePointBox(a, box));
  best2 = std::min(best2, squaredDistancePointBox(b, box));
  best2 = std::min(best2, squaredDistancePointBox(c, box));
  result.approximate_distance = safeSqrt(best2);
  result.intersects =
      containsPoint(box, a) || containsPoint(box, b) || containsPoint(box, c) ||
      result.approximate_distance <= 1.0e-12;
  return result;
}

BoxTriangleDistanceResult BoxTriangleDistance::estimate(
    const AABB& box,
    const TriangleMesh& mesh,
    const MeshTriangle& triangle) {
  if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
      !validIndex(mesh, triangle.v2)) {
    BoxTriangleDistanceResult result;
    result.lower_bound_distance = std::numeric_limits<double>::infinity();
    result.approximate_distance = std::numeric_limits<double>::infinity();
    return result;
  }
  return estimate(
      box,
      toVector3(mesh.vertices[triangle.v0]),
      toVector3(mesh.vertices[triangle.v1]),
      toVector3(mesh.vertices[triangle.v2]));
}

}  // namespace adasdf

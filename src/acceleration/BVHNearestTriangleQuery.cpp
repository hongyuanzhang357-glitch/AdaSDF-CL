#include "adasdf/acceleration/BVHNearestTriangleQuery.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "adasdf/mesh/TriangleDistance.h"

namespace adasdf {
namespace {

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

double distanceSquaredToTriangle(
    const TriangleMesh& mesh,
    int triangle_index,
    const Vector3& point) {
  if (triangle_index < 0 ||
      static_cast<std::size_t>(triangle_index) >= mesh.triangles.size()) {
    return std::numeric_limits<double>::infinity();
  }
  const MeshTriangle& triangle =
      mesh.triangles[static_cast<std::size_t>(triangle_index)];
  if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
      !validIndex(mesh, triangle.v2)) {
    return std::numeric_limits<double>::infinity();
  }
  return pointTriangleSquaredDistance(
      point,
      toVector3(mesh.vertices[triangle.v0]),
      toVector3(mesh.vertices[triangle.v1]),
      toVector3(mesh.vertices[triangle.v2]));
}

}  // namespace

BVHNearestTriangleQueryResult BVHNearestTriangleQuery::query(
    const TriangleBVH& bvh,
    const Vector3& point,
    const BVHNearestTriangleQueryOptions& options) {
  BVHNearestTriangleQueryResult result;
  if (!bvh.isValid() || !point.allFinite() || bvh.mesh() == nullptr) {
    return result;
  }
  const TriangleMesh& mesh = *bvh.mesh();
  double best2 = std::numeric_limits<double>::infinity();
  if (std::isfinite(options.max_distance) && options.max_distance >= 0.0) {
    best2 = options.max_distance * options.max_distance;
  }

  std::vector<int> stack;
  stack.reserve(64);
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_index = stack.back();
    stack.pop_back();
    if (node_index < 0 ||
        static_cast<std::size_t>(node_index) >= bvh.nodes().size()) {
      continue;
    }
    const TriangleBVHNode& node =
        bvh.nodes()[static_cast<std::size_t>(node_index)];
    ++result.node_visits;
    if (squaredDistanceToPoint(node.bounds, point) > best2) {
      continue;
    }
    if (node.isLeaf()) {
      const std::size_t end = node.start + node.count;
      for (std::size_t i = node.start; i < end &&
                              i < bvh.triangleIndices().size();
           ++i) {
        const int triangle_index = bvh.triangleIndices()[i];
        const double d2 = distanceSquaredToTriangle(mesh, triangle_index, point);
        ++result.triangle_tests;
        if (!std::isfinite(d2)) {
          continue;
        }
        constexpr double eps = 1e-18;
        if (d2 < best2 - eps ||
            (std::abs(d2 - best2) <= eps &&
             (result.triangle_index < 0 ||
              triangle_index < result.triangle_index))) {
          best2 = d2;
          result.triangle_index = triangle_index;
          result.distance_squared = d2;
          result.distance = std::sqrt(std::max(0.0, d2));
          result.success = true;
        }
      }
      continue;
    }

    const int left = node.left;
    const int right = node.right;
    const double left_d2 =
        left >= 0 && static_cast<std::size_t>(left) < bvh.nodes().size()
            ? squaredDistanceToPoint(
                  bvh.nodes()[static_cast<std::size_t>(left)].bounds,
                  point)
            : std::numeric_limits<double>::infinity();
    const double right_d2 =
        right >= 0 && static_cast<std::size_t>(right) < bvh.nodes().size()
            ? squaredDistanceToPoint(
                  bvh.nodes()[static_cast<std::size_t>(right)].bounds,
                  point)
            : std::numeric_limits<double>::infinity();
    if (left_d2 <= right_d2) {
      if (right_d2 <= best2) {
        stack.push_back(right);
      }
      if (left_d2 <= best2) {
        stack.push_back(left);
      }
    } else {
      if (left_d2 <= best2) {
        stack.push_back(left);
      }
      if (right_d2 <= best2) {
        stack.push_back(right);
      }
    }
  }

  return result;
}

}  // namespace adasdf

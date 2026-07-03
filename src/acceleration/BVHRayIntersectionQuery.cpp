#include "adasdf/acceleration/BVHRayIntersectionQuery.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace adasdf {
namespace {

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

bool rayIntersectsAABB(
    const BVHRay& ray,
    const TriangleAABB& box,
    double epsilon) {
  if (!isValid(box) || !ray.origin.allFinite() || !ray.direction.allFinite()) {
    return false;
  }
  double tmin = 0.0;
  double tmax = std::numeric_limits<double>::infinity();
  const double o[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
  const double d[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
  const double lo[3] = {box.min.x, box.min.y, box.min.z};
  const double hi[3] = {box.max.x, box.max.y, box.max.z};
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(d[axis]) <= epsilon) {
      if (o[axis] < lo[axis] || o[axis] > hi[axis]) {
        return false;
      }
      continue;
    }
    double t0 = (lo[axis] - o[axis]) / d[axis];
    double t1 = (hi[axis] - o[axis]) / d[axis];
    if (t0 > t1) {
      std::swap(t0, t1);
    }
    tmin = std::max(tmin, t0);
    tmax = std::min(tmax, t1);
    if (tmax < tmin) {
      return false;
    }
  }
  return tmax >= std::max(0.0, tmin);
}

bool rayIntersectsTriangle(
    const BVHRay& ray,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c,
    double epsilon,
    double* t,
    bool* edge_hit) {
  const Vector3 ab = b - a;
  const Vector3 ac = c - a;
  const Vector3 pvec = cross(ray.direction, ac);
  const double det = dot(ab, pvec);
  if (std::abs(det) <= epsilon) {
    return false;
  }
  const double inv_det = 1.0 / det;
  const Vector3 tvec = ray.origin - a;
  const double u = dot(tvec, pvec) * inv_det;
  if (u < -epsilon || u > 1.0 + epsilon) {
    return false;
  }
  const Vector3 qvec = cross(tvec, ab);
  const double v = dot(ray.direction, qvec) * inv_det;
  if (v < -epsilon || u + v > 1.0 + epsilon) {
    return false;
  }
  const double hit_t = dot(ac, qvec) * inv_det;
  if (hit_t <= epsilon) {
    return false;
  }
  if (t != nullptr) {
    *t = hit_t;
  }
  if (edge_hit != nullptr) {
    *edge_hit = std::abs(u) <= epsilon || std::abs(v) <= epsilon ||
                std::abs(1.0 - u - v) <= epsilon;
  }
  return true;
}

void addUniqueHit(
    std::vector<double>* hits,
    double value,
    double epsilon) {
  for (double existing : *hits) {
    if (std::abs(existing - value) <= epsilon) {
      return;
    }
  }
  hits->push_back(value);
}

}  // namespace

BVHRayIntersectionResult BVHRayIntersectionQuery::countIntersections(
    const TriangleBVH& bvh,
    const BVHRay& ray,
    const BVHRayIntersectionOptions& options) {
  BVHRayIntersectionResult result;
  if (!bvh.isValid() || bvh.mesh() == nullptr ||
      !ray.origin.allFinite() || !ray.direction.allFinite()) {
    return result;
  }
  const TriangleMesh& mesh = *bvh.mesh();
  std::vector<double> hits;
  hits.reserve(8);
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
    if (!rayIntersectsAABB(ray, node.bounds, options.epsilon)) {
      continue;
    }
    if (node.isLeaf()) {
      const std::size_t end = node.start + node.count;
      for (std::size_t i = node.start; i < end &&
                              i < bvh.triangleIndices().size();
           ++i) {
        const int triangle_index = bvh.triangleIndices()[i];
        if (triangle_index < 0 ||
            static_cast<std::size_t>(triangle_index) >=
                mesh.triangles.size()) {
          continue;
        }
        const MeshTriangle& tri =
            mesh.triangles[static_cast<std::size_t>(triangle_index)];
        if (!validIndex(mesh, tri.v0) || !validIndex(mesh, tri.v1) ||
            !validIndex(mesh, tri.v2)) {
          continue;
        }
        double t = 0.0;
        bool edge_hit = false;
        ++result.triangle_tests;
        if (rayIntersectsTriangle(
                ray,
                toVector3(mesh.vertices[tri.v0]),
                toVector3(mesh.vertices[tri.v1]),
                toVector3(mesh.vertices[tri.v2]),
                options.epsilon,
                &t,
                &edge_hit)) {
          if (edge_hit) {
            result.ambiguous = true;
          }
          if (options.count_unique_intersections) {
            addUniqueHit(&hits, t, options.unique_t_epsilon);
          } else {
            hits.push_back(t);
          }
        }
      }
    } else {
      if (node.right >= 0) {
        stack.push_back(node.right);
      }
      if (node.left >= 0) {
        stack.push_back(node.left);
      }
    }
  }
  result.hit_count = hits.size();
  result.success = true;
  return result;
}

}  // namespace adasdf

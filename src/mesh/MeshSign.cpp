#include "adasdf/mesh/MeshSign.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "adasdf/mesh/TriangleDistance.h"

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

bool rayTriangleIntersectionT(
    const Vector3& origin,
    const Vector3& direction,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c,
    double eps,
    double& t_out) {
  const Vector3 ab = b - a;
  const Vector3 ac = c - a;
  const Vector3 pvec = cross(direction, ac);
  const double det = dot(ab, pvec);
  if (std::abs(det) <= eps) {
    return false;
  }
  const double inv_det = 1.0 / det;
  const Vector3 tvec = origin - a;
  const double u = dot(tvec, pvec) * inv_det;
  if (u < -eps || u > 1.0 + eps) {
    return false;
  }
  const Vector3 qvec = cross(tvec, ab);
  const double v = dot(direction, qvec) * inv_det;
  if (v < -eps || u + v > 1.0 + eps) {
    return false;
  }
  const double t = dot(ac, qvec) * inv_det;
  if (t <= eps || !std::isfinite(t)) {
    return false;
  }
  t_out = t;
  return true;
}

int uniqueCrossingCount(std::vector<double> hits, double eps) {
  if (hits.empty()) {
    return 0;
  }
  std::sort(hits.begin(), hits.end());
  int count = 0;
  double last = -std::numeric_limits<double>::infinity();
  const double merge_eps = std::max(eps * 16.0, 1.0e-12);
  for (const double t : hits) {
    if (count == 0 || std::abs(t - last) > merge_eps) {
      ++count;
      last = t;
    }
  }
  return count;
}

}  // namespace

MeshSignResult MeshSign::classifyPoint(
    const TriangleMesh& mesh,
    const Vector3& p,
    const MeshSignOptions& options) {
  if (mesh.empty() || !p.allFinite()) {
    return MeshSignResult::Ambiguous;
  }
  const double eps =
      (options.ray_epsilon > 0.0 && std::isfinite(options.ray_epsilon))
          ? options.ray_epsilon
          : 1.0e-12;

  for (const MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      return MeshSignResult::Ambiguous;
    }
    const Vector3 a = toVector3(mesh.vertices[triangle.v0]);
    const Vector3 b = toVector3(mesh.vertices[triangle.v1]);
    const Vector3 c = toVector3(mesh.vertices[triangle.v2]);
    if (pointTriangleDistance(p, a, b, c) <= eps * 16.0) {
      return MeshSignResult::OnSurface;
    }
  }

  const Vector3 directions[] = {
      {1.0, 0.0, 0.0},
      {1.0, 1.0e-7, 3.0e-7},
      {1.0, -2.0e-7, 5.0e-7}};
  const int tries = std::clamp(options.max_retry_rays, 1, 3);
  bool saw_valid = false;
  bool inside = false;
  for (int ray_id = 0; ray_id < tries; ++ray_id) {
    std::vector<double> hits;
    bool ambiguous = false;
    for (const MeshTriangle& triangle : mesh.triangles) {
      const Vector3 a = toVector3(mesh.vertices[triangle.v0]);
      const Vector3 b = toVector3(mesh.vertices[triangle.v1]);
      const Vector3 c = toVector3(mesh.vertices[triangle.v2]);
      double t = 0.0;
      if (rayTriangleIntersectionT(p, directions[ray_id], a, b, c, eps, t)) {
        hits.push_back(t);
      }
      if (!std::isfinite(t)) {
        ambiguous = true;
      }
    }
    if (ambiguous) {
      continue;
    }
    const int crossings = uniqueCrossingCount(std::move(hits), eps);
    const bool current_inside = (crossings % 2) == 1;
    if (!saw_valid) {
      saw_valid = true;
      inside = current_inside;
    } else if (inside != current_inside) {
      return MeshSignResult::Ambiguous;
    }
  }
  if (!saw_valid) {
    return MeshSignResult::Ambiguous;
  }
  return inside ? MeshSignResult::Inside : MeshSignResult::Outside;
}

const char* toString(MeshSignResult result) {
  switch (result) {
    case MeshSignResult::Outside:
      return "outside";
    case MeshSignResult::Inside:
      return "inside";
    case MeshSignResult::OnSurface:
      return "on_surface";
    case MeshSignResult::Ambiguous:
      return "ambiguous";
  }
  return "unknown";
}

}  // namespace adasdf

#include "adasdf/audit/NearSurfaceSampleGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
  return {a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

double norm(const Vector3& v) {
  return std::sqrt(std::max(0.0, dot(v, v)));
}

Vector3 normalized(const Vector3& v) {
  const double len = norm(v);
  if (!(len > 0.0) || !std::isfinite(len)) {
    return Vector3::Zero();
  }
  return v / len;
}

double triangleArea(const TriangleMesh& mesh, const MeshTriangle& triangle) {
  if (triangle.v0 < 0 || triangle.v1 < 0 || triangle.v2 < 0 ||
      static_cast<std::size_t>(triangle.v0) >= mesh.vertices.size() ||
      static_cast<std::size_t>(triangle.v1) >= mesh.vertices.size() ||
      static_cast<std::size_t>(triangle.v2) >= mesh.vertices.size()) {
    return 0.0;
  }
  const Vector3 a = toVector3(mesh.vertices[triangle.v0]);
  const Vector3 b = toVector3(mesh.vertices[triangle.v1]);
  const Vector3 c = toVector3(mesh.vertices[triangle.v2]);
  const double area = 0.5 * norm(cross(b - a, c - a));
  return std::isfinite(area) ? area : 0.0;
}

Vector3 triangleNormal(const TriangleMesh& mesh, const MeshTriangle& triangle) {
  const Vector3 a = toVector3(mesh.vertices[triangle.v0]);
  const Vector3 b = toVector3(mesh.vertices[triangle.v1]);
  const Vector3 c = toVector3(mesh.vertices[triangle.v2]);
  return normalized(cross(b - a, c - a));
}

double fract(double value) {
  return value - std::floor(value);
}

double sequenceValue(std::size_t index, double multiplier, unsigned int seed) {
  const double shifted =
      (static_cast<double>(index) + 1.0) * multiplier +
      static_cast<double>((seed % 997u) + 1u) * 0.0009765625;
  return fract(shifted);
}

Vector3 barycentricPoint(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle,
    std::size_t index,
    unsigned int seed) {
  const Vector3 a = toVector3(mesh.vertices[triangle.v0]);
  const Vector3 b = toVector3(mesh.vertices[triangle.v1]);
  const Vector3 c = toVector3(mesh.vertices[triangle.v2]);
  const double u = sequenceValue(index, 0.7548776662466927, seed);
  const double v = sequenceValue(index, 0.5698402909980532, seed ^ 0x9e3779b9u);
  const double su = std::sqrt(std::clamp(u, 0.0, 1.0));
  const double w0 = 1.0 - su;
  const double w1 = su * (1.0 - v);
  const double w2 = su * v;
  return w0 * a + w1 * b + w2 * c;
}

}  // namespace

NearSurfaceSampleSet NearSurfaceSampleGenerator::generate(
    const TriangleMesh& mesh,
    const NearSurfaceSampleOptions& options) {
  NearSurfaceSampleSet result;
  result.requested_surface_sample_count = options.surface_sample_count;
  if (mesh.triangles.empty() || mesh.vertices.empty() ||
      options.surface_sample_count == 0 || options.offsets.empty()) {
    return result;
  }

  struct WeightedTriangle {
    int triangle_index = -1;
    double cumulative_area = 0.0;
  };

  std::vector<WeightedTriangle> weighted;
  weighted.reserve(mesh.triangles.size());
  double total_area = 0.0;
  for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
    const double area = triangleArea(mesh, mesh.triangles[i]);
    if (!(area > 0.0) || !std::isfinite(area)) {
      continue;
    }
    total_area += area;
    weighted.push_back({static_cast<int>(i), total_area});
  }
  result.total_surface_area = total_area;
  if (weighted.empty() || !(total_area > 0.0)) {
    return result;
  }

  result.samples.reserve(options.surface_sample_count * options.offsets.size());
  for (std::size_t s = 0; s < options.surface_sample_count; ++s) {
    const double area_target =
        (static_cast<double>(s) + 0.5) * total_area /
        static_cast<double>(options.surface_sample_count);
    const auto iter = std::lower_bound(
        weighted.begin(),
        weighted.end(),
        area_target,
        [](const WeightedTriangle& item, double value) {
          return item.cumulative_area < value;
        });
    const WeightedTriangle& selected =
        iter == weighted.end() ? weighted.back() : *iter;
    const MeshTriangle& triangle =
        mesh.triangles[static_cast<std::size_t>(selected.triangle_index)];
    const Vector3 normal = triangleNormal(mesh, triangle);
    if (!normal.allFinite() || norm(normal) <= 0.0) {
      continue;
    }
    const Vector3 surface_point =
        barycentricPoint(mesh, triangle, s, options.seed);
    for (double offset : options.offsets) {
      NearSurfaceSample sample;
      sample.surface_sample_id = s;
      sample.triangle_index = selected.triangle_index;
      sample.surface_point = surface_point;
      sample.triangle_normal = normal;
      sample.offset = offset;
      sample.point = surface_point + offset * normal;
      result.samples.push_back(sample);
    }
    ++result.generated_surface_sample_count;
  }
  return result;
}

}  // namespace adasdf

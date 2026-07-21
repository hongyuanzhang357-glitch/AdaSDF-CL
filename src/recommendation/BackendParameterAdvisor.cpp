#include "adasdf/recommendation/BackendParameterAdvisor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

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

double length(const Vector3& value) {
  return std::sqrt(dot(value, value));
}

std::uint64_t fnvUpdate(
    std::uint64_t value,
    const void* data,
    std::size_t size) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < size; ++i) {
    value ^= static_cast<std::uint64_t>(bytes[i]);
    value *= 1099511628211ull;
  }
  return value;
}

std::string meshHash(const TriangleMesh& mesh) {
  std::uint64_t hash = 1469598103934665603ull;
  for (const MeshVertex& vertex : mesh.vertices) {
    hash = fnvUpdate(hash, &vertex, sizeof(vertex));
  }
  for (const MeshTriangle& triangle : mesh.triangles) {
    hash = fnvUpdate(hash, &triangle, sizeof(triangle));
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << hash;
  return out.str();
}

double percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const std::size_t index = static_cast<std::size_t>(std::llround(
      std::clamp(q, 0.0, 1.0) * static_cast<double>(values.size() - 1)));
  return values[index];
}

}  // namespace

GeometryFeatures GeometryFeatureExtractor::fromMesh(
    const TriangleMesh& mesh,
    const MeshDiagnosticsReport& diagnostics) {
  GeometryFeatures out;
  out.mesh_hash = meshHash(mesh);
  out.vertex_count = mesh.vertexCount();
  out.triangle_count = mesh.triangleCount();
  const MeshAABB bounds = mesh.aabb();
  out.aabb_extent = {
      bounds.max.x - bounds.min.x,
      bounds.max.y - bounds.min.y,
      bounds.max.z - bounds.min.z};
  out.aabb_diagonal = mesh.diagonalLength();
  const double min_extent = std::min({
      out.aabb_extent.x, out.aabb_extent.y, out.aabb_extent.z});
  const double max_extent = std::max({
      out.aabb_extent.x, out.aabb_extent.y, out.aabb_extent.z});
  out.aspect_ratio = min_extent > 0.0
      ? max_extent / min_extent
      : std::numeric_limits<double>::infinity();
  std::vector<double> edges;
  Vector3 normal_sum{};
  std::size_t normal_count = 0;
  double signed_volume = 0.0;
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (triangle.v0 < 0 || triangle.v1 < 0 || triangle.v2 < 0 ||
        static_cast<std::size_t>(triangle.v0) >= mesh.vertices.size() ||
        static_cast<std::size_t>(triangle.v1) >= mesh.vertices.size() ||
        static_cast<std::size_t>(triangle.v2) >= mesh.vertices.size()) {
      continue;
    }
    const Vector3 a =
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v0)]);
    const Vector3 b =
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v1)]);
    const Vector3 c =
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v2)]);
    edges.push_back(length(b - a));
    edges.push_back(length(c - b));
    edges.push_back(length(a - c));
    const Vector3 raw_normal = cross(b - a, c - a);
    const double twice_area = length(raw_normal);
    out.surface_area += 0.5 * twice_area;
    signed_volume += dot(a, cross(b, c)) / 6.0;
    if (twice_area > 1.0e-15) {
      normal_sum = normal_sum + raw_normal / twice_area;
      ++normal_count;
    }
  }
  out.absolute_volume_proxy = std::abs(signed_volume);
  if (!edges.empty()) {
    out.edge_length_min = *std::min_element(edges.begin(), edges.end());
    out.edge_length_max = *std::max_element(edges.begin(), edges.end());
    out.edge_length_mean =
        std::accumulate(edges.begin(), edges.end(), 0.0) /
        static_cast<double>(edges.size());
    out.edge_length_p95 = percentile(edges, 0.95);
  }
  out.normal_variation = normal_count > 0
      ? std::clamp(
            1.0 - length(normal_sum) / static_cast<double>(normal_count),
            0.0,
            1.0)
      : 0.0;
  const double density = out.aabb_diagonal > 0.0
      ? std::log1p(static_cast<double>(out.triangle_count)) /
          out.aabb_diagonal
      : 0.0;
  out.geometry_complexity =
      out.normal_variation * std::log1p(std::max(0.0, density));
  out.watertight = diagnostics.watertight;
  out.valid = diagnostics.valid_mesh && diagnostics.watertight &&
      out.triangle_count > 0 && std::isfinite(out.aspect_ratio);
  return out;
}

BackendParameterAdvice HeuristicBackendParameterAdvisor::advise(
    const GeometryFeatures& features,
    const SDFCreationConstraints& constraints) const {
  BackendParameterAdvice out;
  out.source = "heuristic-v1";
  if (!features.valid ||
      !validateSDFCreationConstraints(constraints, nullptr)) {
    out.warnings.push_back("invalid features or hard constraints");
    return out;
  }
  const double cells = features.aabb_diagonal /
      std::max(constraints.max_zero_surface_abs_error, 1.0e-15);
  out.parameters.octree_min_depth = 1;
  out.parameters.octree_max_depth = std::clamp(
      static_cast<int>(std::ceil(std::log2(std::max(2.0, cells)))), 2, 7);
  out.parameters.octree_narrow_band_scale = 1.5;
  out.parameters.octree_interpolation_residual_scale = 0.25;
  out.parameters.octree_max_overlapping_triangles =
      features.geometry_complexity > 1.0 ? 8 : 16;
  out.parameters.octree_max_normal_complexity = 0.15;
  out.parameters.bvh_leaf_size = features.triangle_count > 100000 ? 16 : 8;
  out.parameters.block_min_nodes = 4;
  out.parameters.block_max_nodes =
      constraints.max_decoded_block_bytes < 64 * 1024 ? 32 : 128;
  out.parameters.block_target_nodes =
      std::min(32, out.parameters.block_max_nodes);
  out.parameters.block_halo = 0;
  out.parameters.block_min_tensor_dimension = 3;
  out.parameters.block_max_tensor_dimension = 33;
  out.parameters.compression_method =
      features.geometry_complexity > 1.0 ? "TT" : "Tucker";
  out.parameters.compression_abs_tolerance =
      constraints.max_zero_surface_abs_error * 0.25;
  out.parameters.compression_max_rank =
      features.geometry_complexity > 1.0 ? 12 : 8;
  out.success = true;
  return out;
}

}  // namespace adasdf

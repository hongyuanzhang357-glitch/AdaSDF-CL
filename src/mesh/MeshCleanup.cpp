#include "adasdf/mesh/MeshCleanup.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <unordered_map>

namespace adasdf {

namespace {

struct VertexKey {
  long long x = 0;
  long long y = 0;
  long long z = 0;

  bool operator==(const VertexKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  bool operator<(const VertexKey& other) const {
    if (x != other.x) {
      return x < other.x;
    }
    if (y != other.y) {
      return y < other.y;
    }
    return z < other.z;
  }
};

struct VertexKeyHash {
  std::size_t operator()(const VertexKey& key) const {
    std::size_t h = std::hash<long long>{}(key.x);
    h ^= std::hash<long long>{}(key.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<long long>{}(key.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
  }
};

VertexKey makeKey(const MeshVertex& vertex, double tolerance) {
  if (!(tolerance > 0.0) || !std::isfinite(tolerance)) {
    tolerance = std::numeric_limits<double>::epsilon();
  }
  return {
      static_cast<long long>(std::llround(vertex.x / tolerance)),
      static_cast<long long>(std::llround(vertex.y / tolerance)),
      static_cast<long long>(std::llround(vertex.z / tolerance))};
}

MeshVertex sub(const MeshVertex& a, const MeshVertex& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

MeshVertex cross(const MeshVertex& a, const MeshVertex& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
}

double norm(const MeshVertex& vertex) {
  return std::sqrt(vertex.x * vertex.x + vertex.y * vertex.y +
                   vertex.z * vertex.z);
}

bool finite(const MeshVertex& vertex) {
  return std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
         std::isfinite(vertex.z);
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

bool degenerate(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle,
    double area_epsilon) {
  if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
      !validIndex(mesh, triangle.v2)) {
    return true;
  }
  if (triangle.v0 == triangle.v1 || triangle.v0 == triangle.v2 ||
      triangle.v1 == triangle.v2) {
    return true;
  }
  const MeshVertex& a = mesh.vertices[triangle.v0];
  const MeshVertex& b = mesh.vertices[triangle.v1];
  const MeshVertex& c = mesh.vertices[triangle.v2];
  if (!finite(a) || !finite(b) || !finite(c)) {
    return true;
  }
  const double area = 0.5 * norm(cross(sub(b, a), sub(c, a)));
  return area < area_epsilon;
}

struct TriangleKey {
  std::array<VertexKey, 3> vertices;

  bool operator<(const TriangleKey& other) const {
    return vertices < other.vertices;
  }
};

TriangleKey makeTriangleKey(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle,
    double tolerance) {
  TriangleKey key;
  key.vertices = {
      makeKey(mesh.vertices[triangle.v0], tolerance),
      makeKey(mesh.vertices[triangle.v1], tolerance),
      makeKey(mesh.vertices[triangle.v2], tolerance)};
  std::sort(key.vertices.begin(), key.vertices.end());
  return key;
}

TriangleMesh mergeVertices(
    const TriangleMesh& mesh,
    const MeshCleanupOptions& options,
    MeshCleanupStats& stats) {
  if (!options.merge_near_duplicate_vertices) {
    return mesh;
  }

  TriangleMesh out;
  std::unordered_map<VertexKey, int, VertexKeyHash> remap_by_key;
  std::vector<int> old_to_new(mesh.vertices.size(), -1);
  for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
    const VertexKey key = makeKey(mesh.vertices[i], options.vertex_merge_tolerance);
    const auto found = remap_by_key.find(key);
    if (found != remap_by_key.end()) {
      old_to_new[i] = found->second;
      ++stats.merged_vertices;
      continue;
    }
    out.vertices.push_back(mesh.vertices[i]);
    const int new_index = static_cast<int>(out.vertices.size() - 1);
    remap_by_key[key] = new_index;
    old_to_new[i] = new_index;
  }

  out.triangles.reserve(mesh.triangles.size());
  for (const MeshTriangle& triangle : mesh.triangles) {
    MeshTriangle mapped = triangle;
    mapped.v0 = validIndex(mesh, triangle.v0) ? old_to_new[triangle.v0] : -1;
    mapped.v1 = validIndex(mesh, triangle.v1) ? old_to_new[triangle.v1] : -1;
    mapped.v2 = validIndex(mesh, triangle.v2) ? old_to_new[triangle.v2] : -1;
    out.triangles.push_back(mapped);
  }
  return out;
}

TriangleMesh removeDegenerateTriangles(
    const TriangleMesh& mesh,
    const MeshCleanupOptions& options,
    MeshCleanupStats& stats) {
  if (!options.remove_degenerate_triangles) {
    return mesh;
  }

  TriangleMesh out;
  out.vertices = mesh.vertices;
  out.triangles.reserve(mesh.triangles.size());
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (degenerate(mesh, triangle, options.degenerate_area_epsilon)) {
      ++stats.removed_degenerate_triangles;
      continue;
    }
    out.triangles.push_back(triangle);
  }
  return out;
}

TriangleMesh removeDuplicateTriangles(
    const TriangleMesh& mesh,
    const MeshCleanupOptions& options,
    MeshCleanupStats& stats) {
  if (!options.remove_duplicate_triangles) {
    return mesh;
  }

  TriangleMesh out;
  out.vertices = mesh.vertices;
  out.triangles.reserve(mesh.triangles.size());
  std::map<TriangleKey, int> seen;
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      out.triangles.push_back(triangle);
      continue;
    }
    const TriangleKey key =
        makeTriangleKey(mesh, triangle, options.vertex_merge_tolerance);
    if (seen.find(key) != seen.end()) {
      ++stats.removed_duplicate_triangles;
      continue;
    }
    seen[key] = static_cast<int>(out.triangles.size());
    out.triangles.push_back(triangle);
  }
  return out;
}

TriangleMesh removeUnusedVertices(
    const TriangleMesh& mesh,
    const MeshCleanupOptions& options,
    MeshCleanupStats& stats) {
  if (!options.remove_unused_vertices) {
    return mesh;
  }

  std::vector<int> old_to_new(mesh.vertices.size(), -1);
  std::vector<bool> used(mesh.vertices.size(), false);
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (validIndex(mesh, triangle.v0)) {
      used[triangle.v0] = true;
    }
    if (validIndex(mesh, triangle.v1)) {
      used[triangle.v1] = true;
    }
    if (validIndex(mesh, triangle.v2)) {
      used[triangle.v2] = true;
    }
  }

  TriangleMesh out;
  for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
    if (!used[i]) {
      ++stats.removed_unused_vertices;
      continue;
    }
    old_to_new[i] = static_cast<int>(out.vertices.size());
    out.vertices.push_back(mesh.vertices[i]);
  }

  out.triangles.reserve(mesh.triangles.size());
  for (const MeshTriangle& triangle : mesh.triangles) {
    MeshTriangle mapped = triangle;
    mapped.v0 = validIndex(mesh, triangle.v0) ? old_to_new[triangle.v0] : -1;
    mapped.v1 = validIndex(mesh, triangle.v1) ? old_to_new[triangle.v1] : -1;
    mapped.v2 = validIndex(mesh, triangle.v2) ? old_to_new[triangle.v2] : -1;
    out.triangles.push_back(mapped);
  }
  return out;
}

}  // namespace

MeshCleanupResult MeshCleanup::clean(
    const TriangleMesh& mesh,
    const MeshCleanupOptions& options) {
  MeshCleanupResult result;
  result.stats.input_vertices = mesh.vertexCount();
  result.stats.input_triangles = mesh.triangleCount();

  if (mesh.vertices.empty() || mesh.triangles.empty()) {
    result.error_message = "cannot clean an empty mesh";
    return result;
  }

  if (options.fill_holes) {
    result.stats.warnings.push_back(
        "fill_holes is not implemented in v1.4 and was ignored");
  }
  if (options.repair_self_intersections) {
    result.stats.warnings.push_back(
        "repair_self_intersections is not implemented in v1.4 and was ignored");
  }

  TriangleMesh current = mesh;
  current = mergeVertices(current, options, result.stats);
  current = removeDegenerateTriangles(current, options, result.stats);
  current = removeDuplicateTriangles(current, options, result.stats);
  current = removeUnusedVertices(current, options, result.stats);

  result.stats.output_vertices = current.vertexCount();
  result.stats.output_triangles = current.triangleCount();
  result.stats.topology_may_have_changed =
      result.stats.merged_vertices > 0 ||
      result.stats.removed_degenerate_triangles > 0 ||
      result.stats.removed_duplicate_triangles > 0;

  if (result.stats.topology_may_have_changed) {
    result.stats.warnings.push_back(
        "cleanup removed or merged mesh elements; rerun diagnostics and "
        "readiness before SDF construction");
  }
  if (current.triangles.empty()) {
    result.error_message = "cleanup removed all triangles";
    result.cleaned_mesh = current;
    return result;
  }

  result.cleaned_mesh = current;
  result.success = true;
  return result;
}

}  // namespace adasdf

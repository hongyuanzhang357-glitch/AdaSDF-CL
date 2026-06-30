#include "adasdf/mesh/MeshDiagnostics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_map>

namespace adasdf {

namespace {

constexpr std::size_t kMaxSamples = 8;

bool finite(const MeshVertex& vertex) {
  return std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
         std::isfinite(vertex.z);
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

double triangleArea2(
    const MeshVertex& a,
    const MeshVertex& b,
    const MeshVertex& c) {
  return norm(cross(sub(b, a), sub(c, a)));
}

struct EdgeKey {
  int a = -1;
  int b = -1;

  bool operator<(const EdgeKey& other) const {
    if (a != other.a) {
      return a < other.a;
    }
    return b < other.b;
  }
};

EdgeKey edgeKey(int a, int b) {
  return {std::min(a, b), std::max(a, b)};
}

struct QuantizedVertex {
  long long x = 0;
  long long y = 0;
  long long z = 0;

  bool operator<(const QuantizedVertex& other) const {
    if (x != other.x) {
      return x < other.x;
    }
    if (y != other.y) {
      return y < other.y;
    }
    return z < other.z;
  }
};

QuantizedVertex quantize(const MeshVertex& vertex, double tolerance) {
  if (!(tolerance > 0.0) || !std::isfinite(tolerance)) {
    tolerance = std::numeric_limits<double>::epsilon();
  }
  return {
      static_cast<long long>(std::llround(vertex.x / tolerance)),
      static_cast<long long>(std::llround(vertex.y / tolerance)),
      static_cast<long long>(std::llround(vertex.z / tolerance))};
}

struct TriangleKey {
  std::array<QuantizedVertex, 3> vertices;

  bool operator<(const TriangleKey& other) const {
    return vertices < other.vertices;
  }
};

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

TriangleKey triangleKey(
    const TriangleMesh& mesh,
    const MeshTriangle& triangle,
    double tolerance) {
  TriangleKey key;
  key.vertices = {
      quantize(mesh.vertices[triangle.v0], tolerance),
      quantize(mesh.vertices[triangle.v1], tolerance),
      quantize(mesh.vertices[triangle.v2], tolerance)};
  std::sort(key.vertices.begin(), key.vertices.end());
  return key;
}

int findParent(std::vector<int>& parent, int value) {
  while (parent[value] != value) {
    parent[value] = parent[parent[value]];
    value = parent[value];
  }
  return value;
}

void unite(std::vector<int>& parent, int a, int b) {
  const int ra = findParent(parent, a);
  const int rb = findParent(parent, b);
  if (ra != rb) {
    parent[rb] = ra;
  }
}

void addSample(std::vector<int>& samples, int value) {
  if (samples.size() < kMaxSamples) {
    samples.push_back(value);
  }
}

void addEdgeSample(std::vector<std::pair<int, int>>& samples, EdgeKey edge) {
  if (samples.size() < kMaxSamples) {
    samples.push_back({edge.a, edge.b});
  }
}

}  // namespace

MeshDiagnosticsReport MeshDiagnostics::analyze(
    const TriangleMesh& mesh,
    const MeshDiagnosticsOptions& options) {
  MeshDiagnosticsReport report;
  report.vertex_count = mesh.vertexCount();
  report.triangle_count = mesh.triangleCount();
  report.raw_triangle_count = mesh.triangleCount();
  report.aabb = mesh.aabb();
  report.diagonal_length = mesh.diagonalLength();

  if (mesh.vertices.empty()) {
    report.errors.push_back("mesh has no vertices");
  }
  if (mesh.triangles.empty()) {
    report.errors.push_back("mesh has no triangles");
  }

  std::vector<bool> vertex_used(mesh.vertices.size(), false);
  std::map<EdgeKey, std::vector<int>> edge_faces;
  std::map<TriangleKey, int> seen_triangles;
  std::vector<int> parent(mesh.triangles.size());
  std::iota(parent.begin(), parent.end(), 0);

  for (std::size_t face_index = 0; face_index < mesh.triangles.size();
       ++face_index) {
    const MeshTriangle& triangle = mesh.triangles[face_index];
    const int sample_id = triangle.original_face_id >= 0
                              ? triangle.original_face_id
                              : static_cast<int>(face_index);
    const bool valid_indices =
        validIndex(mesh, triangle.v0) && validIndex(mesh, triangle.v1) &&
        validIndex(mesh, triangle.v2);
    if (!valid_indices) {
      ++report.degenerate_triangle_count;
      ++report.zero_area_face_count;
      addSample(report.sample_degenerate_faces, sample_id);
      report.has_nan_or_inf = true;
      continue;
    }

    vertex_used[triangle.v0] = true;
    vertex_used[triangle.v1] = true;
    vertex_used[triangle.v2] = true;

    const MeshVertex& a = mesh.vertices[triangle.v0];
    const MeshVertex& b = mesh.vertices[triangle.v1];
    const MeshVertex& c = mesh.vertices[triangle.v2];
    const bool repeated =
        triangle.v0 == triangle.v1 || triangle.v0 == triangle.v2 ||
        triangle.v1 == triangle.v2;
    const bool non_finite = !finite(a) || !finite(b) || !finite(c);
    const double area = 0.5 * triangleArea2(a, b, c);
    if (non_finite) {
      report.has_nan_or_inf = true;
    }
    if (options.check_degenerate_triangles &&
        (repeated || non_finite || area < options.degenerate_area_epsilon)) {
      ++report.degenerate_triangle_count;
      ++report.zero_area_face_count;
      addSample(report.sample_degenerate_faces, sample_id);
    }

    if (options.check_duplicate_triangles) {
      const TriangleKey key =
          triangleKey(mesh, triangle, options.duplicate_triangle_tolerance);
      const auto found = seen_triangles.find(key);
      if (found != seen_triangles.end()) {
        ++report.duplicate_triangle_count;
        addSample(report.sample_duplicate_faces, sample_id);
      } else {
        seen_triangles[key] = static_cast<int>(face_index);
      }
    }

    const std::array<EdgeKey, 3> edges = {
        edgeKey(triangle.v0, triangle.v1),
        edgeKey(triangle.v1, triangle.v2),
        edgeKey(triangle.v2, triangle.v0)};
    for (const EdgeKey& edge : edges) {
      edge_faces[edge].push_back(static_cast<int>(face_index));
    }
  }

  for (bool used : vertex_used) {
    if (!used) {
      ++report.isolated_vertex_count;
    }
  }

  if (options.check_boundary_edges || options.check_non_manifold_edges) {
    for (const auto& item : edge_faces) {
      const EdgeKey edge = item.first;
      const std::vector<int>& faces = item.second;
      if (options.check_boundary_edges && faces.size() == 1) {
        ++report.boundary_edge_count;
        addEdgeSample(report.sample_boundary_edges, edge);
      }
      if (options.check_non_manifold_edges && faces.size() >= 3) {
        ++report.non_manifold_edge_count;
        addEdgeSample(report.sample_non_manifold_edges, edge);
      }
      if (options.check_connected_components && faces.size() >= 2) {
        for (std::size_t i = 1; i < faces.size(); ++i) {
          unite(parent, faces[0], faces[i]);
        }
      }
    }
  }

  if (options.check_connected_components && !mesh.triangles.empty()) {
    std::set<int> components;
    for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
      components.insert(findParent(parent, static_cast<int>(i)));
    }
    report.connected_component_count = components.size();
  } else {
    report.connected_component_count = mesh.triangles.empty() ? 0 : 1;
  }

  report.has_small_scale_warning =
      report.diagonal_length > 0.0 && report.diagonal_length < 1e-6;
  report.has_extreme_scale_warning = report.diagonal_length > 1e6;

  if (report.has_nan_or_inf) {
    report.errors.push_back("mesh contains NaN, Inf, or invalid triangle indices");
  }
  if (report.degenerate_triangle_count > 0) {
    report.errors.push_back("mesh contains degenerate triangles");
  }
  if (report.boundary_edge_count > 0) {
    report.warnings.push_back("mesh has boundary edges and is not watertight");
  }
  if (report.non_manifold_edge_count > 0) {
    report.errors.push_back("mesh has non-manifold edges");
  }
  if (report.duplicate_triangle_count > 0) {
    report.warnings.push_back("mesh has duplicate triangles");
  }
  if (report.isolated_vertex_count > 0) {
    report.warnings.push_back("mesh has isolated vertices");
  }
  if (report.connected_component_count > 1) {
    report.warnings.push_back("mesh has multiple connected components");
  }
  if (report.has_small_scale_warning) {
    report.warnings.push_back("mesh AABB diagonal is very small; confirm units");
  }
  if (report.has_extreme_scale_warning) {
    report.warnings.push_back("mesh AABB diagonal is very large; confirm units");
  }

  report.watertight = report.boundary_edge_count == 0 &&
                      report.non_manifold_edge_count == 0 &&
                      report.triangle_count > 0;
  report.valid_mesh = report.triangle_count > 0 && !report.has_nan_or_inf &&
                      report.degenerate_triangle_count == 0 &&
                      report.non_manifold_edge_count == 0;
  report.likely_oriented = report.watertight && report.degenerate_triangle_count == 0;

  if (report.errors.empty() && report.warnings.empty()) {
    report.recommendation =
        "Mesh diagnostics passed. This is a preflight result, not a full "
        "self-intersection or SDF build guarantee.";
  } else if (!report.errors.empty()) {
    report.recommendation =
        "Fix critical mesh issues before using this STL for SDF construction.";
  } else {
    report.recommendation =
        "Review warnings before SDF construction; no automatic repair was "
        "performed.";
  }

  return report;
}

}  // namespace adasdf

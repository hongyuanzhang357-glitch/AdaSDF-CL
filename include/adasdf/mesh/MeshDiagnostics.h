#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct MeshDiagnosticsOptions {
  double degenerate_area_epsilon = 1e-14;
  double duplicate_triangle_tolerance = 1e-12;
  double normal_consistency_epsilon = 1e-12;
  bool check_duplicate_triangles = true;
  bool check_non_manifold_edges = true;
  bool check_boundary_edges = true;
  bool check_degenerate_triangles = true;
  bool check_connected_components = true;
};

struct MeshDiagnosticsReport {
  bool valid_mesh = false;
  bool watertight = false;
  bool likely_oriented = false;

  std::size_t vertex_count = 0;
  std::size_t triangle_count = 0;
  std::size_t raw_triangle_count = 0;

  MeshAABB aabb;
  double diagonal_length = 0.0;

  std::size_t degenerate_triangle_count = 0;
  std::size_t duplicate_triangle_count = 0;
  std::size_t boundary_edge_count = 0;
  std::size_t non_manifold_edge_count = 0;
  std::size_t connected_component_count = 0;

  std::size_t isolated_vertex_count = 0;
  std::size_t zero_area_face_count = 0;

  bool has_nan_or_inf = false;
  bool has_extreme_scale_warning = false;
  bool has_small_scale_warning = false;

  std::vector<int> sample_degenerate_faces;
  std::vector<int> sample_duplicate_faces;
  std::vector<std::pair<int, int>> sample_boundary_edges;
  std::vector<std::pair<int, int>> sample_non_manifold_edges;

  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  std::string recommendation;
};

class MeshDiagnostics {
 public:
  static MeshDiagnosticsReport analyze(
      const TriangleMesh& mesh,
      const MeshDiagnosticsOptions& options = MeshDiagnosticsOptions{});
};

}  // namespace adasdf

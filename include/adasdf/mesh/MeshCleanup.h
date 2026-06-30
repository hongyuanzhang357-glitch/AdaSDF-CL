#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct MeshCleanupOptions {
  bool merge_near_duplicate_vertices = true;
  double vertex_merge_tolerance = 1e-12;

  bool remove_degenerate_triangles = true;
  double degenerate_area_epsilon = 1e-14;

  bool remove_duplicate_triangles = true;
  bool remove_unused_vertices = true;

  bool preserve_face_order_when_possible = true;

  bool fill_holes = false;
  bool repair_self_intersections = false;
};

struct MeshCleanupStats {
  std::size_t input_vertices = 0;
  std::size_t input_triangles = 0;

  std::size_t output_vertices = 0;
  std::size_t output_triangles = 0;

  std::size_t merged_vertices = 0;
  std::size_t removed_degenerate_triangles = 0;
  std::size_t removed_duplicate_triangles = 0;
  std::size_t removed_unused_vertices = 0;

  bool topology_may_have_changed = false;

  std::vector<std::string> warnings;
};

struct MeshCleanupResult {
  TriangleMesh cleaned_mesh;
  MeshCleanupStats stats;
  bool success = false;
  std::string error_message;
};

class MeshCleanup {
 public:
  static MeshCleanupResult clean(
      const TriangleMesh& mesh,
      const MeshCleanupOptions& options = MeshCleanupOptions{});
};

}  // namespace adasdf

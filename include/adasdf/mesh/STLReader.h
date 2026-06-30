#pragma once

#include <cstddef>
#include <string>

#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct STLReadOptions {
  bool merge_duplicate_vertices = true;
  double vertex_merge_tolerance = 1e-12;
  bool keep_original_face_order = true;
};

struct STLReadResult {
  TriangleMesh mesh;
  bool success = false;
  bool is_binary = false;
  std::string error_message;
  std::size_t raw_triangle_count = 0;
  std::size_t unique_vertex_count = 0;
};

class STLReader {
 public:
  static STLReadResult read(
      const std::string& path,
      const STLReadOptions& options = STLReadOptions{});
};

}  // namespace adasdf

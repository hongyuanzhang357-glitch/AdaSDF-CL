#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/acceleration/TriangleAABB.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

enum class TriangleBVHBuildMethod {
  MedianSplit,
  SAHPreview,
};

struct TriangleBVHBuildOptions {
  int max_leaf_size = 8;
  int max_depth = 64;
  bool ignore_degenerate = true;
  double degenerate_area_epsilon = 1e-14;
  TriangleBVHBuildMethod method = TriangleBVHBuildMethod::MedianSplit;
};

struct TriangleBVHBuildReport {
  bool success = false;
  std::string error_message;
  std::size_t triangle_count = 0;
  std::size_t skipped_triangle_count = 0;
  std::size_t node_count = 0;
  std::size_t leaf_count = 0;
  int max_depth_used = 0;
  double build_time_ms = 0.0;
  std::vector<std::string> warnings;
};

struct TriangleBVHNode {
  TriangleAABB bounds;
  int left = -1;
  int right = -1;
  std::size_t start = 0;
  std::size_t count = 0;

  bool isLeaf() const { return left < 0 && right < 0; }
};

struct TriangleBVHRef {
  int triangle_index = -1;
  TriangleAABB bounds;
  Vector3 centroid;
};

class TriangleBVH {
 public:
  bool isValid() const;
  bool empty() const { return nodes_.empty(); }
  const TriangleMesh* mesh() const { return mesh_; }
  const std::vector<TriangleBVHNode>& nodes() const { return nodes_; }
  const std::vector<int>& triangleIndices() const { return triangle_indices_; }
  const TriangleBVHBuildOptions& options() const { return options_; }
  std::size_t nodeCount() const { return nodes_.size(); }
  std::size_t leafCount() const;
  std::size_t triangleCount() const { return triangle_indices_.size(); }

 private:
  friend class TriangleBVHBuilder;
  friend class TriangleBVHInternalBuilder;

  const TriangleMesh* mesh_ = nullptr;
  TriangleBVHBuildOptions options_;
  std::vector<TriangleBVHNode> nodes_;
  std::vector<int> triangle_indices_;
};

}  // namespace adasdf

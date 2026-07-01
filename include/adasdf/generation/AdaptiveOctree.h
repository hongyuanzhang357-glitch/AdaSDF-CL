#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct AdaptiveOctreeNode {
  int id = -1;
  int parent_id = -1;
  int level = 0;

  AABB bounds;

  bool is_leaf = true;
  bool near_surface = false;
  bool contains_surface = false;

  double center_phi = 0.0;
  double min_abs_sample_phi = 0.0;
  double max_abs_sample_phi = 0.0;
  double cell_diagonal = 0.0;

  std::array<int, 8> children;

  AdaptiveOctreeNode();
};

struct AdaptiveOctree {
  std::vector<AdaptiveOctreeNode> nodes;
  int root_id = -1;

  std::vector<int> leafNodeIds() const;
  std::size_t leafCount() const;
  std::size_t nodeCount() const;
  int maxLevelUsed() const;
};

}  // namespace adasdf

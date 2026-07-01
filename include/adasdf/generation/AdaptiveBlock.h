#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct AdaptiveSDFBlock {
  int block_id = -1;
  int octree_node_id = -1;
  int level = 0;

  AABB bounds;

  int nx = 0;
  int ny = 0;
  int nz = 0;

  Vector3 origin;
  Vector3 spacing;

  bool near_surface = false;
  bool signed_distance = true;

  std::vector<double> phi;
};

struct AdaptiveSDFBlockSet {
  std::vector<AdaptiveSDFBlock> blocks;
  AABB global_bounds;
  bool signed_distance = true;

  std::size_t blockCount() const;
  std::size_t memoryFootprintBytes() const;
};

}  // namespace adasdf

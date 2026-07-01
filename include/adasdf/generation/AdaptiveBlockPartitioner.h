#pragma once

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/generation/AdaptiveOctree.h"

namespace adasdf {

struct AdaptiveBlockPartitionOptions {
  int block_resolution = 8;
  bool include_all_leaves = true;
  bool mark_near_surface_blocks = true;
};

class AdaptiveBlockPartitioner {
 public:
  static AdaptiveSDFBlockSet partition(
      const AdaptiveOctree& octree,
      const AdaptiveBlockPartitionOptions& options);
};

}  // namespace adasdf

#pragma once

#include <cstddef>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/generation/AdaptiveBlock.h"

namespace adasdf {

struct ExactBlockSamplingKernelOptions {
  bool signed_distance = true;
  bool enable_counters = false;
  bool enable_diagnostics = false;
};

struct ExactBlockSamplingKernelStats {
  std::size_t sample_count = 0;
  std::size_t distance_query_count = 0;
  std::size_t sign_query_count = 0;
  double time_ms = 0.0;
};

class ExactBlockSamplerKernel {
 public:
  static AdaptiveSDFBlock sample(
      const AABB& block_bounds,
      int block_id,
      int octree_node_id,
      int level,
      int block_resolution,
      BVHSDFSampler& sampler,
      const ExactBlockSamplingKernelOptions& options,
      ExactBlockSamplingKernelStats* stats = nullptr);
};

}  // namespace adasdf

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/sampling/HierarchicalSamplingPolicy.h"
#include "adasdf/sampling/SamplingQualityGuard.h"

namespace adasdf {

struct HierarchicalBlockSamplingStats {
  std::size_t block_count = 0;
  std::size_t exact_block_count = 0;
  std::size_t predicted_block_count = 0;
  std::size_t accepted_prediction_block_count = 0;
  std::size_t fallback_exact_block_count = 0;

  std::size_t exact_sample_count = 0;
  std::size_t predicted_sample_count = 0;
  std::size_t quality_check_sample_count = 0;

  double exact_sampling_time_ms = 0.0;
  double prediction_time_ms = 0.0;
  double quality_check_time_ms = 0.0;
  double total_time_ms = 0.0;

  double speedup_vs_exact_estimate = 0.0;

  std::vector<std::string> warnings;
};

struct HierarchicalBlockSamplingResult {
  bool success = false;
  std::string error_message;

  AdaptiveSDFBlock block;

  BlockSamplingDecision decision;
  SamplingQualityReport quality;

  std::size_t exact_sample_count = 0;
  std::size_t predicted_sample_count = 0;
  std::size_t coarse_sample_count = 0;

  double exact_sampling_time_ms = 0.0;
  double prediction_time_ms = 0.0;
  double quality_check_time_ms = 0.0;
  double total_time_ms = 0.0;

  bool used_prediction = false;
  bool fallback_exact = false;
};

class HierarchicalBlockSampler {
 public:
  static HierarchicalBlockSamplingResult sampleBlock(
      const AABB& block_bounds,
      int block_id,
      int octree_node_id,
      int level,
      int block_resolution,
      bool signed_distance,
      bool near_surface,
      BVHSDFSampler& exact_sampler,
      const HierarchicalSamplingOptions& options);

  static AdaptiveSDFBlock sampleBlockExact(
      const AABB& block_bounds,
      int block_id,
      int octree_node_id,
      int level,
      int block_resolution,
      bool signed_distance,
      bool near_surface,
      BVHSDFSampler& exact_sampler);
};

}  // namespace adasdf

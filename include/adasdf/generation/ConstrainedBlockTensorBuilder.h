#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/generation/ConstrainedBlockPartitioner.h"

namespace adasdf {

enum class SDFTensorNodeSource {
  ExactBVH,
  CoarseInterpolated,
  PromotedExactBVH,
};

const char* toString(SDFTensorNodeSource source);

struct ConstrainedBlockTensorOptions {
  double promote_near_zero_distance = 0.0;
};

struct ConstrainedBlockTensor {
  ConstrainedSDFBlockPlan plan;
  std::vector<double> phi;
  std::vector<SDFTensorNodeSource> source;
};

struct ConstrainedBlockTensorStats {
  bool success = false;
  std::string error_message;
  std::size_t tensor_node_count = 0;
  std::size_t exact_bvh_node_count = 0;
  std::size_t coarse_interpolated_node_count = 0;
  std::size_t promoted_exact_node_count = 0;
  std::size_t promotion_candidate_count = 0;
  std::size_t sign_mismatch_count = 0;
  double max_interpolation_abs_error = 0.0;
  double interpolation_time_ms = 0.0;
};

class ConstrainedBlockTensorBuilder {
 public:
  static ConstrainedBlockTensor build(
      const ConstrainedAdaptiveOctree& tree,
      const ConstrainedSDFBlockPlan& block,
      ExactSDFOracle* oracle,
      const ConstrainedBlockTensorOptions& options,
      ConstrainedBlockTensorStats* stats = nullptr);

  static bool auditAndPromote(
      const ConstrainedAdaptiveOctree& tree,
      ConstrainedBlockTensor* tensor,
      ExactSDFOracle* oracle,
      double max_abs_error,
      ConstrainedBlockTensorStats* stats = nullptr);
};

}  // namespace adasdf

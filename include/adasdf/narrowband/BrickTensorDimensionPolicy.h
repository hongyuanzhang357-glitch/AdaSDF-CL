#pragma once

#include <cstddef>
#include <string>

#include "adasdf/narrowband/CompressionBrick.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"

namespace adasdf {

struct BrickTensorDimensionDecision {
  int tensor_nx = 0;
  int tensor_ny = 0;
  int tensor_nz = 0;
  std::size_t tensor_node_count = 0;
  std::size_t estimated_exact_source_node_count = 0;
  std::size_t estimated_interpolated_fill_node_count = 0;
  std::size_t estimated_expanded_bytes = 0;
  bool warning_too_large = false;
  bool warning_too_small = false;
  bool should_split = false;
  bool should_merge = false;
  std::string reason = "target";
};

class BrickTensorDimensionPolicy {
 public:
  static BrickTensorDimensionDecision decide(
      const CompressionBrick& brick,
      const NarrowBandBrickBuildOptions& options);
};

}  // namespace adasdf

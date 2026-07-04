#pragma once

#include <string>
#include <vector>

#include "adasdf/generation/AdaptiveBlock.h"

namespace adasdf {

enum class BlockImportanceClass {
  NearSurface,
  Transition,
  FarField
};

const char* toString(BlockImportanceClass importance);

struct BlockClassificationOptions {
  double near_surface_band = 1e-3;
  double transition_band = 1e-2;

  bool use_sign_change = true;
  bool use_min_abs_phi = true;
  bool use_block_diagonal_scale = true;

  double near_surface_diagonal_factor = 1.0;
  double transition_diagonal_factor = 3.0;
};

struct BlockClassificationResult {
  BlockImportanceClass importance = BlockImportanceClass::FarField;

  double min_abs_phi = 0.0;
  double max_abs_phi = 0.0;
  bool has_sign_change = false;

  std::vector<std::string> warnings;
};

class BlockClassifier {
 public:
  static BlockClassificationResult classify(
      const AdaptiveSDFBlock& block_probe,
      const BlockClassificationOptions& options = BlockClassificationOptions{});
};

}  // namespace adasdf

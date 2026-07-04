#include "adasdf/sampling/BlockClassification.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

double diagonalLength(const AABB& bounds) {
  if (!bounds.valid) {
    return 0.0;
  }
  const Vector3 d = bounds.max - bounds.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

}  // namespace

const char* toString(BlockImportanceClass importance) {
  switch (importance) {
    case BlockImportanceClass::NearSurface:
      return "near-surface";
    case BlockImportanceClass::Transition:
      return "transition";
    case BlockImportanceClass::FarField:
      return "far-field";
  }
  return "far-field";
}

BlockClassificationResult BlockClassifier::classify(
    const AdaptiveSDFBlock& block_probe,
    const BlockClassificationOptions& options) {
  BlockClassificationResult result;
  if (block_probe.phi.empty()) {
    result.warnings.push_back("empty block probe; classified as far-field");
    return result;
  }

  result.min_abs_phi = std::numeric_limits<double>::infinity();
  result.max_abs_phi = 0.0;
  bool has_positive = false;
  bool has_negative = false;
  for (const double phi : block_probe.phi) {
    if (!std::isfinite(phi)) {
      result.warnings.push_back(
          "non-finite block probe phi; classified as near-surface");
      result.importance = BlockImportanceClass::NearSurface;
      result.min_abs_phi = 0.0;
      return result;
    }
    const double abs_phi = std::abs(phi);
    result.min_abs_phi = std::min(result.min_abs_phi, abs_phi);
    result.max_abs_phi = std::max(result.max_abs_phi, abs_phi);
    has_positive = has_positive || phi > 0.0;
    has_negative = has_negative || phi < 0.0;
  }
  if (!std::isfinite(result.min_abs_phi)) {
    result.min_abs_phi = 0.0;
  }
  result.has_sign_change = has_positive && has_negative;

  const double diag = options.use_block_diagonal_scale
                          ? diagonalLength(block_probe.bounds)
                          : 0.0;
  const double scale = options.use_block_diagonal_scale
                           ? std::max(1.0, diag)
                           : 1.0;
  const double near_band =
      options.near_surface_band *
      std::max(0.0, options.near_surface_diagonal_factor) * scale;
  const double transition_band =
      options.transition_band *
      std::max(0.0, options.transition_diagonal_factor) * scale;

  if ((options.use_sign_change && result.has_sign_change) ||
      (options.use_min_abs_phi && result.min_abs_phi <= near_band)) {
    result.importance = BlockImportanceClass::NearSurface;
  } else if (options.use_min_abs_phi &&
             result.min_abs_phi <= std::max(near_band, transition_band)) {
    result.importance = BlockImportanceClass::Transition;
  } else {
    result.importance = BlockImportanceClass::FarField;
  }
  return result;
}

}  // namespace adasdf

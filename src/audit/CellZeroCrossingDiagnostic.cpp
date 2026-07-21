#include "adasdf/audit/CellZeroCrossingDiagnostic.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

int signWithEps(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

bool hasSignChange(const std::vector<double>& values, double eps) {
  bool has_pos = false;
  bool has_neg = false;
  for (double value : values) {
    const int sign = signWithEps(value, eps);
    has_pos = has_pos || sign > 0;
    has_neg = has_neg || sign < 0;
  }
  return has_pos && has_neg;
}

}  // namespace

CellZeroCrossingResult CellZeroCrossingDiagnostic::diagnose(
    const CellZeroCrossingInput& input) {
  CellZeroCrossingResult result;
  bool has_pos_corner = false;
  bool has_neg_corner = false;
  int first_nonzero_corner_sign = 0;
  for (double phi : input.exact_corner_phi) {
    const int sign = signWithEps(phi, input.sign_epsilon);
    if (sign != 0 && first_nonzero_corner_sign == 0) {
      first_nonzero_corner_sign = sign;
    }
    has_pos_corner = has_pos_corner || sign > 0;
    has_neg_corner = has_neg_corner || sign < 0;
  }
  result.exact_corners_mixed_sign = has_pos_corner && has_neg_corner;
  result.exact_corners_same_sign =
      !result.exact_corners_mixed_sign &&
      (has_pos_corner || has_neg_corner);
  const int center_sign =
      signWithEps(input.exact_center_phi, input.sign_epsilon);
  result.exact_center_opposite_corner_sign =
      result.exact_corners_same_sign && center_sign != 0 &&
      first_nonzero_corner_sign != 0 &&
      center_sign != first_nonzero_corner_sign;
  result.exact_stencil_sign_change =
      hasSignChange(input.exact_stencil_phi, input.sign_epsilon);

  const bool geometric_overlap =
      input.triangle_aabb_overlap ||
      input.expanded_triangle_aabb_overlap ||
      input.nearest_surface_point_inside_cell;
  result.likely_zero_crossing_inside_cell =
      geometric_overlap || result.exact_corners_mixed_sign ||
      result.exact_center_opposite_corner_sign ||
      result.exact_stencil_sign_change;

  if (input.block_lookup_suspicious) {
    result.category = "block_lookup_suspicious";
  } else if (input.reference_sign_suspicious) {
    result.category = "reference_sign_suspicious";
  } else if (result.likely_zero_crossing_inside_cell &&
             result.exact_corners_same_sign) {
    result.category = "surface_crossing_with_same_sign_corners";
  } else if (result.likely_zero_crossing_inside_cell &&
             result.exact_corners_mixed_sign) {
    result.category = "surface_crossing_with_mixed_corners";
  } else {
    result.category = "no_surface_crossing_but_sign_mismatch";
  }
  return result;
}

}  // namespace adasdf

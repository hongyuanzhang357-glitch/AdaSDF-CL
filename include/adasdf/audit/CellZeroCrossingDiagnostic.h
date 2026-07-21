#pragma once

#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct CellZeroCrossingInput {
  bool triangle_aabb_overlap = false;
  bool expanded_triangle_aabb_overlap = false;
  bool nearest_surface_point_inside_cell = false;
  bool block_lookup_suspicious = false;
  bool reference_sign_suspicious = false;

  std::vector<double> exact_corner_phi;
  double exact_center_phi = 0.0;
  std::vector<double> exact_stencil_phi;
  double sign_epsilon = 1.0e-9;
};

struct CellZeroCrossingResult {
  bool exact_corners_same_sign = false;
  bool exact_corners_mixed_sign = false;
  bool exact_center_opposite_corner_sign = false;
  bool exact_stencil_sign_change = false;
  bool likely_zero_crossing_inside_cell = false;
  std::string category = "unknown";
};

class CellZeroCrossingDiagnostic {
 public:
  static CellZeroCrossingResult diagnose(
      const CellZeroCrossingInput& input);
};

}  // namespace adasdf

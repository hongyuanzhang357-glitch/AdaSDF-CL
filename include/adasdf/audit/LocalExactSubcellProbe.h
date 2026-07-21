#pragma once

#include <functional>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct LocalExactSubcellProbeResult {
  int subgrid = 0;
  double phi = 0.0;
  int sign = 0;
  bool fixed_mismatch = false;
  bool remaining_mismatch = false;
};

class LocalExactSubcellProbe {
 public:
  using ExactPhiFunction = std::function<double(const Vector3&)>;

  static LocalExactSubcellProbeResult probe(
      const AABB& cell_bounds,
      const Vector3& query_point,
      int original_sdf_sign,
      int reference_sign,
      int subgrid,
      const ExactPhiFunction& exact_phi,
      double sign_epsilon = 1.0e-9);
};

}  // namespace adasdf

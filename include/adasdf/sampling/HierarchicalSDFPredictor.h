#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct SDFPredictionResult {
  bool success = false;
  std::string error_message;

  std::vector<double> predicted_phi;

  int nx = 0;
  int ny = 0;
  int nz = 0;

  std::size_t exact_sample_count = 0;
  std::size_t predicted_sample_count = 0;

  std::vector<std::string> warnings;
};

class HierarchicalSDFPredictor {
 public:
  static SDFPredictionResult predictFromCoarseSamples(
      const AABB& block_bounds,
      int nx,
      int ny,
      int nz,
      const std::vector<double>& coarse_phi,
      int coarse_nx,
      int coarse_ny,
      int coarse_nz);
};

double interpolateGridPhi(
    const AABB& bounds,
    int nx,
    int ny,
    int nz,
    const std::vector<double>& phi,
    const Vector3& point);

}  // namespace adasdf

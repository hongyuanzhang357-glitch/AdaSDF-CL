#pragma once

#include "adasdf/backend/Backend.h"

namespace adasdf {

struct DistanceRequest {
  BackendType backend = BackendType::CPU;
  QueryMode query_mode = QueryMode::Balanced;
  Scalar distance_tolerance = 1.0e-6;
  bool enable_nearest_points = true;
  bool enable_gradient = true;
  bool enable_jacobian = false;
  int max_iterations = 64;
};

}  // namespace adasdf

#pragma once

#include "adasdf/backend/Backend.h"
#include "adasdf/query/QueryModeConfig.h"

namespace adasdf {

struct DistanceRequest {
  BackendType backend = BackendType::CPU;
  QueryMode query_mode = QueryMode::Balanced;
  QueryModeConfig query_mode_config = QueryModeConfig::cpuDirect();
  Scalar distance_tolerance = 1.0e-6;
  bool enable_nearest_points = true;
  bool enable_gradient = true;
  bool enable_jacobian = false;
  int max_iterations = 64;
};

}  // namespace adasdf

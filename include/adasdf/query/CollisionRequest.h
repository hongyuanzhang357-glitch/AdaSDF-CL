#pragma once

#include "adasdf/backend/Backend.h"
#include "adasdf/query/QueryModeConfig.h"

namespace adasdf {

struct CollisionRequest {
  bool enable_contact = false;
  int max_contacts = 1;
  Scalar contact_tolerance = 1.0e-4;
  bool enable_gradient = false;
  bool enable_jacobian = false;
  bool enable_contact_reduction = true;
  int candidate_grid_resolution = 8;
  int max_candidate_points = 4096;
  Scalar near_surface_band = 1.0e-3;
  Scalar contact_merge_tolerance = 1.0e-4;
  Scalar normal_merge_cosine = 0.95;
  bool symmetric_query = true;
  BackendType backend = BackendType::CPU;
  QueryMode query_mode = QueryMode::Balanced;
  QueryModeConfig query_mode_config = QueryModeConfig::cpuDirect();
  BroadphaseType broadphase = BroadphaseType::Auto;
  NarrowphaseType narrowphase = NarrowphaseType::AdaptiveSDF;
};

}  // namespace adasdf

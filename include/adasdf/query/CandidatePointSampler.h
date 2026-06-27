#pragma once

#include <vector>

#include "adasdf/query/CollisionObject.h"

namespace adasdf {

enum class CandidateSamplingMode {
  AABBCorners,
  AABBGrid,
  SurfaceApprox,
  NearSurfaceApprox,
  Auto
};

struct CandidateSamplingOptions {
  CandidateSamplingMode mode = CandidateSamplingMode::Auto;
  int grid_resolution = 8;
  int max_points = 4096;
  Scalar surface_band = 1.0e-3;
  bool include_aabb_corners = true;
  bool include_aabb_face_centers = true;
  bool include_aabb_grid = true;
};

class CandidatePointSampler {
 public:
  static std::vector<Vector3> sampleWorldPoints(
      const CollisionObject& object,
      const CandidateSamplingOptions& options);
};

}  // namespace adasdf

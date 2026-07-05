#pragma once

#include "adasdf/geometry/Transform.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct BoxTriangleDistanceResult {
  double lower_bound_distance = 0.0;
  double approximate_distance = 0.0;
  bool intersects = false;
};

class BoxTriangleDistance {
 public:
  static BoxTriangleDistanceResult estimate(
      const AABB& box,
      const Vector3& a,
      const Vector3& b,
      const Vector3& c);

  static BoxTriangleDistanceResult estimate(
      const AABB& box,
      const TriangleMesh& mesh,
      const MeshTriangle& triangle);
};

}  // namespace adasdf

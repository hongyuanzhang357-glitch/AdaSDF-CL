#pragma once

#include <cstddef>

#include "adasdf/acceleration/TriangleBVH.h"

namespace adasdf {

struct BVHRay {
  Vector3 origin;
  Vector3 direction;
};

struct BVHRayIntersectionOptions {
  double epsilon = 1e-12;
  double unique_t_epsilon = 1e-9;
  bool count_unique_intersections = true;
};

struct BVHRayIntersectionResult {
  bool success = false;
  std::size_t hit_count = 0;
  bool ambiguous = false;
  std::size_t node_visits = 0;
  std::size_t triangle_tests = 0;
};

class BVHRayIntersectionQuery {
 public:
  static BVHRayIntersectionResult countIntersections(
      const TriangleBVH& bvh,
      const BVHRay& ray,
      const BVHRayIntersectionOptions& options =
          BVHRayIntersectionOptions());
};

}  // namespace adasdf

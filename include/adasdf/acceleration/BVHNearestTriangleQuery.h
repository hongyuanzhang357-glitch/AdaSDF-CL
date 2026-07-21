#pragma once

#include <limits>

#include "adasdf/acceleration/TriangleBVH.h"

namespace adasdf {

struct BVHNearestTriangleQueryOptions {
  double max_distance = std::numeric_limits<double>::infinity();
  int initial_triangle_index = -1;
};

struct BVHNearestTriangleQueryResult {
  bool success = false;
  int triangle_index = -1;
  double distance = std::numeric_limits<double>::infinity();
  double distance_squared = std::numeric_limits<double>::infinity();
  std::size_t node_visits = 0;
  std::size_t triangle_tests = 0;
};

class BVHNearestTriangleQuery {
 public:
  static BVHNearestTriangleQueryResult query(
      const TriangleBVH& bvh,
      const Vector3& point,
      const BVHNearestTriangleQueryOptions& options =
          BVHNearestTriangleQueryOptions());
};

}  // namespace adasdf

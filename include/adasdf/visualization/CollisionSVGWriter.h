#pragma once

#include <filesystem>
#include <string>

#include "adasdf/geometry/Transform.h"
#include "adasdf/query/CollisionResult.h"

namespace adasdf {

struct CollisionSVGScene {
  AABB box_a;
  AABB box_b;
  CollisionResult result;
  std::string title = "AdaSDF-CL collision view";
  std::string backend;
  std::string method;
  int requested_max_contacts = 0;
};

class CollisionSVGWriter {
 public:
  static void write(
      const std::filesystem::path& path,
      const CollisionSVGScene& scene);
};

}  // namespace adasdf

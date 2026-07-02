#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct CollisionSample {
  int sample_id = -1;
  Vector3 position;
  double radius = 0.0;
  int object_id = 0;
  int link_id = 0;
  int group_id = 0;
  double weight = 1.0;
  std::string label;
};

struct CollisionSampleSet {
  std::vector<CollisionSample> samples;

  bool empty() const;
  std::size_t size() const;
  AABB boundingBox() const;
  void clear();
  void add(const CollisionSample& sample);
};

}  // namespace adasdf

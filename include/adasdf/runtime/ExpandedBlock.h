#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct ActiveExpandedBlock {
  int block_id = -1;
  int source_block_id = -1;
  int level = 0;
  AABB bounds;
  int nx = 0;
  int ny = 0;
  int nz = 0;
  Vector3 origin;
  Vector3 spacing;
  bool near_surface = false;
  bool signed_distance = true;
  std::vector<double> phi;

  bool isValid() const;
  bool contains(const Vector3& point) const;
  std::size_t valueCount() const;
  std::size_t memoryBytes() const;
  double sampleDistance(const Vector3& point) const;
  Vector3 sampleGradient(const Vector3& point) const;
};

}  // namespace adasdf

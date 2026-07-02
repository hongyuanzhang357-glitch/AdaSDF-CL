#include "adasdf/sparse/CollisionSampleSet.h"

#include <algorithm>

namespace adasdf {

bool CollisionSampleSet::empty() const {
  return samples.empty();
}

std::size_t CollisionSampleSet::size() const {
  return samples.size();
}

AABB CollisionSampleSet::boundingBox() const {
  AABB bounds;
  if (samples.empty()) {
    return bounds;
  }
  const auto expand = [](Vector3& min, Vector3& max, const CollisionSample& sample) {
    const double r = std::max(0.0, sample.radius);
    min.x = std::min(min.x, sample.position.x - r);
    min.y = std::min(min.y, sample.position.y - r);
    min.z = std::min(min.z, sample.position.z - r);
    max.x = std::max(max.x, sample.position.x + r);
    max.y = std::max(max.y, sample.position.y + r);
    max.z = std::max(max.z, sample.position.z + r);
  };
  const double r = std::max(0.0, samples.front().radius);
  bounds.min = {
      samples.front().position.x - r,
      samples.front().position.y - r,
      samples.front().position.z - r};
  bounds.max = {
      samples.front().position.x + r,
      samples.front().position.y + r,
      samples.front().position.z + r};
  for (const CollisionSample& sample : samples) {
    expand(bounds.min, bounds.max, sample);
  }
  bounds.valid = true;
  return bounds;
}

void CollisionSampleSet::clear() {
  samples.clear();
}

void CollisionSampleSet::add(const CollisionSample& sample) {
  samples.push_back(sample);
}

}  // namespace adasdf

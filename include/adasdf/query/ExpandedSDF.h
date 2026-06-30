#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

enum class ExpandedSDFLayout {
  GlobalDense,
  BlockDense
};

struct ExpandedBlock {
  int block_id = -1;
  Vector3 min_corner;
  Vector3 max_corner;
  int resolution_x = 0;
  int resolution_y = 0;
  int resolution_z = 0;
  std::vector<double> values;
};

struct ExpandedSDFQueryPolicy {
  double near_surface_band = 1e-3;
  double sign_epsilon = 1e-9;
  bool clamp_outside_expanded_domain = false;
  bool allow_direct_fallback_outside = true;
};

class ExpandedSDF {
 public:
  ExpandedSDF() = default;

  static ExpandedSDF globalDense(ExpandedBlock block);
  static ExpandedSDF blockDense(std::vector<ExpandedBlock> blocks);

  ExpandedSDFLayout layout() const {
    return layout_;
  }

  bool isValid() const;
  bool hasBlock(int block_id) const;
  const std::vector<int>& blockIds() const {
    return block_ids_;
  }
  const std::vector<ExpandedBlock>& blocks() const {
    return blocks_;
  }
  AABB boundingBox() const;

  void setQueryPolicy(const ExpandedSDFQueryPolicy& policy) {
    policy_ = policy;
  }
  const ExpandedSDFQueryPolicy& queryPolicy() const {
    return policy_;
  }

  std::size_t memoryFootprintBytes() const;

  bool contains(const Vector3& p) const;
  double sampleDistance(const Vector3& p) const;
  Vector3 sampleGradient(const Vector3& p) const;

  std::string description() const;

 private:
  ExpandedSDFLayout layout_ = ExpandedSDFLayout::GlobalDense;
  std::vector<ExpandedBlock> blocks_;
  std::vector<int> block_ids_;
  ExpandedSDFQueryPolicy policy_;
};

const char* toString(ExpandedSDFLayout layout);

}  // namespace adasdf

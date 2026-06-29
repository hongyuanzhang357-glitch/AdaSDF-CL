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

  std::size_t memoryFootprintBytes() const;

  bool contains(const Vector3& p) const;
  double sampleDistance(const Vector3& p) const;
  Vector3 sampleGradient(const Vector3& p) const;

  std::string description() const;

 private:
  ExpandedSDFLayout layout_ = ExpandedSDFLayout::GlobalDense;
  std::vector<ExpandedBlock> blocks_;
  std::vector<int> block_ids_;
};

const char* toString(ExpandedSDFLayout layout);

}  // namespace adasdf

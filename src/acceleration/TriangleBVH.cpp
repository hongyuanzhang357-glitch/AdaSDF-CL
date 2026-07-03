#include "adasdf/acceleration/TriangleBVH.h"

namespace adasdf {

bool TriangleBVH::isValid() const {
  return mesh_ != nullptr && !nodes_.empty() && !triangle_indices_.empty() &&
         adasdf::isValid(nodes_.front().bounds);
}

std::size_t TriangleBVH::leafCount() const {
  std::size_t count = 0;
  for (const TriangleBVHNode& node : nodes_) {
    if (node.isLeaf()) {
      ++count;
    }
  }
  return count;
}

}  // namespace adasdf

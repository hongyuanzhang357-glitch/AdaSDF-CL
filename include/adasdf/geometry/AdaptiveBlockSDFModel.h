#pragma once

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class AdaptiveBlockSDFModel final : public SDFModel {
 public:
  AdaptiveBlockSDFModel();
  explicit AdaptiveBlockSDFModel(AdaptiveSDFBlockSet blocks);

  const AdaptiveSDFBlockSet& blockSet() const;
  AdaptiveSDFBlockSet& blockSet();

  int findContainingBlock(const Vector3& p) const;

  static bool isValidBlockSet(const AdaptiveSDFBlockSet& blocks);
  static const char* backendName();

 private:
  std::shared_ptr<AdaptiveSDFBlockSet> block_set_;
};

}  // namespace adasdf

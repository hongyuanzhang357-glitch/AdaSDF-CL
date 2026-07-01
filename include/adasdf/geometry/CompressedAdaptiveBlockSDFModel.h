#pragma once

#include "adasdf/compression/CompressedSDFBlock.h"
#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class CompressedAdaptiveBlockSDFModel final : public SDFModel {
 public:
  CompressedAdaptiveBlockSDFModel();
  explicit CompressedAdaptiveBlockSDFModel(CompressedAdaptiveBlockSDF compressed);

  const CompressedAdaptiveBlockSDF& compressedBlockSet() const;
  CompressedAdaptiveBlockSDF& compressedBlockSet();

  int findContainingBlock(const Vector3& p) const;

  static bool isValidCompressedBlockSet(
      const CompressedAdaptiveBlockSDF& compressed);
  static const char* backendName();

 private:
  std::shared_ptr<CompressedAdaptiveBlockSDF> compressed_;
};

}  // namespace adasdf

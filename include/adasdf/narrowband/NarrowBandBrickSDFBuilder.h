#pragma once

#include <memory>

#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"
#include "adasdf/narrowband/NarrowBandBrickBuildStats.h"

namespace adasdf {

class NarrowBandBrickSDFBuilder {
 public:
  static std::shared_ptr<AdaptiveBlockSDFModel> fromMesh(
      const TriangleMesh& mesh,
      const NarrowBandBrickBuildOptions& options,
      NarrowBandBrickBuildStats* stats = nullptr);
};

}  // namespace adasdf

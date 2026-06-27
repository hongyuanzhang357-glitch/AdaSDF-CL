#include "adasdf/generation/SDFBuilder.h"

#include <stdexcept>

namespace adasdf {

std::shared_ptr<SDFModel> SDFBuilder::fromMesh(
    const std::filesystem::path&,
    const BuildOptions&) {
  throw std::runtime_error(
      "SDFBuilder::fromMesh is not implemented in v0.2; use existing sdf_tool "
      "to build .sdfbin fixtures.");
}

std::shared_ptr<SDFModel> SDFBuilder::fromMesh(
    const MeshModel&,
    const BuildOptions&) {
  throw std::runtime_error("SDFBuilder::fromMesh(MeshModel) is not implemented in v0.2.");
}

}  // namespace adasdf

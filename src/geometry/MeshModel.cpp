#include "adasdf/geometry/MeshModel.h"

#include <stdexcept>

namespace adasdf {

MeshModel::MeshModel(std::filesystem::path source_path)
    : source_path_(std::move(source_path)) {}

AABB MeshModel::localAABB() const {
  return bounds_;
}

std::string MeshModel::debugName() const {
  return source_path_.empty() ? "MeshModel" : source_path_.string();
}

MeshModel MeshModel::fromFile(const std::filesystem::path& path) {
  throw std::runtime_error(
      "MeshModel::fromFile is not implemented in v0.2; use existing sdf_tool "
      "to build a .sdfbin, then load it with SDFBinReader.");
}

}  // namespace adasdf

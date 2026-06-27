#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/generation/BuildOptions.h"
#include "adasdf/geometry/MeshModel.h"
#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class SDFBuilder {
 public:
  // TODO: connect to sdf::StlReader, sdf::SignedDistance, and dense grid paths.
  static std::shared_ptr<SDFModel> fromMesh(
      const std::filesystem::path& mesh_path,
      const BuildOptions& options = {});

  static std::shared_ptr<SDFModel> fromMesh(
      const MeshModel& mesh,
      const BuildOptions& options = {});
};

}  // namespace adasdf

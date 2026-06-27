#pragma once

#include <string>

#include "adasdf/generation/SDFBuilder.h"

namespace adasdf {

class AdaptiveSDFBuilder {
 public:
  static std::shared_ptr<SDFModel> fromMesh(
      const std::string& mesh_path,
      const BuildOptions& options = {});

  static std::shared_ptr<SDFModel> fromSTL(
      const std::string& stl_path,
      const BuildOptions& options = {});

  static std::shared_ptr<SDFModel> fromOBJ(
      const std::string& obj_path,
      const BuildOptions& options = {});

  static std::shared_ptr<SDFModel> fromMesh(
      const MeshModel& mesh,
      const BuildOptions& options = {});
};

}  // namespace adasdf

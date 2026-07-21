#pragma once

#include <filesystem>

#include "adasdf/generation/SDFCreationContract.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

class ConstrainedSDFBuilder {
 public:
  static SDFCreationResult fromSTL(
      const std::filesystem::path& stl_path,
      const std::filesystem::path& output_path,
      const SDFCreationConstraints& constraints);

  static SDFCreationResult fromMesh(
      const TriangleMesh& mesh,
      const std::filesystem::path& output_path,
      const SDFCreationConstraints& constraints);

  static SDFCreationResult fromSTLWithParameters(
      const std::filesystem::path& stl_path,
      const std::filesystem::path& output_path,
      const SDFCreationConstraints& constraints,
      const BackendBuildParameters& parameters);

  static SDFCreationResult fromMeshWithParameters(
      const TriangleMesh& mesh,
      const std::filesystem::path& output_path,
      const SDFCreationConstraints& constraints,
      const BackendBuildParameters& parameters);
};

}  // namespace adasdf

#pragma once

#include <memory>
#include <string>

#include "adasdf/generation/BuildOptions.h"
#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class ExistingBuilderBridge {
 public:
  static bool isAvailable();

  static std::shared_ptr<SDFModel> buildFromMesh(
      const std::string& mesh_path,
      const BuildOptions& options);

  static bool writeSDFBin(
      const std::string& output_path,
      const SDFModel& model);

  static std::string backendDescription();
};

}  // namespace adasdf

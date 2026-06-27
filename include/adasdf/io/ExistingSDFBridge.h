#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class ExistingSDFBridge {
 public:
  static std::shared_ptr<SDFModel> loadExistingSDFBin(
      const std::filesystem::path& path);

  static bool canLoad(const std::filesystem::path& path);

  static SDFMetadata readMetadata(const std::filesystem::path& path);

  static bool existingCoreAvailable();
};

}  // namespace adasdf

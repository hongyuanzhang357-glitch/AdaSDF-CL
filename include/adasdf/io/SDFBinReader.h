#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

class SDFBinReader {
 public:
  static std::shared_ptr<SDFModel> read(const std::filesystem::path& path);
  static std::shared_ptr<SDFModel> read(const std::string& path) {
    return read(std::filesystem::path(path));
  }
};

}  // namespace adasdf

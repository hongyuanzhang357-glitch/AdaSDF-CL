#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/AdaptiveBlockSDFModel.h"

namespace adasdf {

class AdaptiveBlockSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);
  static std::shared_ptr<AdaptiveBlockSDFModel> read(
      const std::filesystem::path& path);
  static void write(
      const std::filesystem::path& path,
      const AdaptiveBlockSDFModel& model);
};

}  // namespace adasdf

#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {

class CompressedBlockSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);

  static std::shared_ptr<CompressedAdaptiveBlockSDFModel> read(
      const std::filesystem::path& path);

  static void write(
      const std::filesystem::path& path,
      const CompressedAdaptiveBlockSDFModel& model);
};

}  // namespace adasdf

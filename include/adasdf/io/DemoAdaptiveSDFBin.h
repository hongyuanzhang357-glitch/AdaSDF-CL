#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/DemoAdaptiveSDFModel.h"

namespace adasdf {

class DemoAdaptiveSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);
  static std::shared_ptr<DemoAdaptiveSDFModel> read(
      const std::filesystem::path& path);
  static void write(
      const std::filesystem::path& path,
      const DemoAdaptiveSDFModel& model);
};

}  // namespace adasdf

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "adasdf/geometry/AnalyticSDFModel.h"

namespace adasdf {

class DemoSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);
  static std::shared_ptr<AnalyticSDFModel> read(const std::filesystem::path& path);
  static void write(
      const std::filesystem::path& path,
      const AnalyticSDFModel& model);
};

}  // namespace adasdf

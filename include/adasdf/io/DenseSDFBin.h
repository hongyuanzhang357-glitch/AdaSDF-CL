#pragma once

#include <filesystem>
#include <memory>

#include "adasdf/geometry/DenseSDFModel.h"

namespace adasdf {

class DenseSDFBin {
 public:
  static const char* magic();
  static bool canRead(const std::filesystem::path& path);
  static std::shared_ptr<DenseSDFModel> read(
      const std::filesystem::path& path);
  static void write(
      const std::filesystem::path& path,
      const DenseSDFModel& model);
};

}  // namespace adasdf

#pragma once

#include <filesystem>
#include <string>

#include "adasdf/narrowband/NarrowBandBrickBuildOptions.h"
#include "adasdf/narrowband/NarrowBandBrickBuildStats.h"

namespace adasdf {

class NarrowBandBrickReportWriter {
 public:
  static std::string toJson(
      const NarrowBandBrickBuildStats& stats,
      const NarrowBandBrickBuildOptions& options);
  static std::string toMarkdown(
      const NarrowBandBrickBuildStats& stats,
      const NarrowBandBrickBuildOptions& options);
  static bool writeJson(
      const std::filesystem::path& path,
      const NarrowBandBrickBuildStats& stats,
      const NarrowBandBrickBuildOptions& options);
  static bool writeMarkdown(
      const std::filesystem::path& path,
      const NarrowBandBrickBuildStats& stats,
      const NarrowBandBrickBuildOptions& options);
  static bool writeProgressJsonl(
      const std::filesystem::path& path,
      const NarrowBandBrickBuildStats& stats);
};

}  // namespace adasdf

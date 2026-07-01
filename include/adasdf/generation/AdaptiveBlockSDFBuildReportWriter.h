#pragma once

#include <string>

#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"

namespace adasdf {

class AdaptiveBlockSDFBuildReportWriter {
 public:
  static std::string toMarkdown(const AdaptiveBlockSDFBuildReport& report);
  static std::string toJson(const AdaptiveBlockSDFBuildReport& report);
  static void writeMarkdown(
      const std::string& path,
      const AdaptiveBlockSDFBuildReport& report);
  static void writeJson(
      const std::string& path,
      const AdaptiveBlockSDFBuildReport& report);
};

}  // namespace adasdf

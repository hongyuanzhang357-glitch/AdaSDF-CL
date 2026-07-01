#pragma once

#include <string>

#include "adasdf/generation/DenseSDFBuilder.h"

namespace adasdf {

class DenseSDFBuildReportWriter {
 public:
  static std::string toMarkdown(const DenseSDFBuildReport& report);
  static std::string toJson(const DenseSDFBuildReport& report);
  static void writeMarkdown(
      const std::string& path,
      const DenseSDFBuildReport& report);
  static void writeJson(
      const std::string& path,
      const DenseSDFBuildReport& report);
};

}  // namespace adasdf

#pragma once

#include <string>

#include "adasdf/generation/AdaptiveTreeStats.h"

namespace adasdf {

class AdaptiveTreeStatsWriter {
 public:
  static std::string toJson(
      const AdaptiveTreeStats& stats,
      const std::string& tool_name = "adasdf_diagnose_adaptive_tree");

  static std::string toMarkdown(const AdaptiveTreeStats& stats);

  static bool writeJson(
      const std::string& path,
      const AdaptiveTreeStats& stats,
      const std::string& tool_name = "adasdf_diagnose_adaptive_tree");

  static bool writeMarkdown(
      const std::string& path,
      const AdaptiveTreeStats& stats);
};

}  // namespace adasdf

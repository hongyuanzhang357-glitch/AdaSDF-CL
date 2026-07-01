#pragma once

#include <string>

#include "adasdf/recommendation/BuildRecommendationTypes.h"

namespace adasdf {

class BuildRecommendationWriter {
 public:
  static std::string toMarkdown(const BuildRecommendationReport& report);
  static std::string toJson(const BuildRecommendationReport& report);

  static void writeMarkdown(
      const std::string& path,
      const BuildRecommendationReport& report);

  static void writeJson(
      const std::string& path,
      const BuildRecommendationReport& report);
};

}  // namespace adasdf

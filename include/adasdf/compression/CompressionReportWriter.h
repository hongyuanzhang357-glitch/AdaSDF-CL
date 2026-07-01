#pragma once

#include <string>

#include "adasdf/compression/BlockLowRankCompressor.h"
#include "adasdf/compression/CompressionQuality.h"

namespace adasdf {

class CompressionReportWriter {
 public:
  static std::string toMarkdown(const BlockLowRankCompressionReport& report);
  static std::string toJson(const BlockLowRankCompressionReport& report);

  static std::string qualityToMarkdown(const CompressionQualityReport& report);
  static std::string qualityToJson(const CompressionQualityReport& report);

  static void writeMarkdown(
      const std::string& path,
      const BlockLowRankCompressionReport& report);
  static void writeJson(
      const std::string& path,
      const BlockLowRankCompressionReport& report);
  static void writeQualityMarkdown(
      const std::string& path,
      const CompressionQualityReport& report);
  static void writeQualityJson(
      const std::string& path,
      const CompressionQualityReport& report);
};

}  // namespace adasdf

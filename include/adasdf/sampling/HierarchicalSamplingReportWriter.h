#pragma once

#include <string>

#include "adasdf/sampling/SpeedQualityBenchmark.h"

namespace adasdf {

class HierarchicalSamplingReportWriter {
 public:
  static std::string toMarkdown(const SpeedQualityBenchmarkResult& result);
  static std::string toJson(const SpeedQualityBenchmarkResult& result);
  static std::string csvHeader();
  static std::string csvRow(const SpeedQualityBenchmarkResult& result);

  static void writeMarkdown(
      const std::string& path,
      const SpeedQualityBenchmarkResult& result);
  static void writeJson(
      const std::string& path,
      const SpeedQualityBenchmarkResult& result);
  static void writeCsv(
      const std::string& path,
      const SpeedQualityBenchmarkResult& result);
};

}  // namespace adasdf

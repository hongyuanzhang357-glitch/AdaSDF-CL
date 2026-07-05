#pragma once

#include <string>

#include "adasdf/sampling/ContactBandDiagnostics.h"
#include "adasdf/sampling/ContactBandQualityAudit.h"

namespace adasdf {

struct ContactBandBenchmarkResult {
  std::string case_id = "contact_band";

  bool success = false;
  std::string error_message;

  double exact_reference_time_ms = 0.0;
  double contact_band_time_ms = 0.0;
  double speedup = 0.0;
  double effective_speedup_including_marker = 0.0;
  double effective_speedup_excluding_marker = 0.0;

  ContactBandDiagnostics diagnostics;
  ContactBandQualityMetrics quality;

  bool effective_speedup_claim_allowed = false;
};

class ContactBandReportWriter {
 public:
  static std::string csvHeader();
  static std::string csvRow(const ContactBandBenchmarkResult& result);
  static std::string toMarkdown(const ContactBandBenchmarkResult& result);
  static void writeCsv(
      const std::string& path,
      const ContactBandBenchmarkResult& result);
  static void writeMarkdown(
      const std::string& path,
      const ContactBandBenchmarkResult& result);
};

}  // namespace adasdf

#pragma once

#include <string>

#include "adasdf/sampling/ContactBandDiagnostics.h"
#include "adasdf/sampling/ContactBandQualityAudit.h"

namespace adasdf {

struct ContactBandBenchmarkResult {
  std::string case_id = "contact_band";

  bool success = false;
  std::string error_message;

  std::string timing_mode = "end-to-end";
  bool include_audit_in_wall_time = true;
  bool include_marker_in_speedup = true;
  bool exclude_audit_from_speedup = false;
  bool exclude_marker_from_speedup = false;

  double exact_reference_time_ms = 0.0;
  double contact_band_time_ms = 0.0;
  double speedup = 0.0;
  double effective_speedup_including_marker = 0.0;
  double effective_speedup_excluding_marker = 0.0;
  double exact_reference_wall_time_ms = 0.0;
  double contact_band_wall_time_ms = 0.0;
  double speedup_end_to_end = 0.0;
  double exact_reference_core_build_time_ms = 0.0;
  double contact_band_core_build_time_ms = 0.0;
  double speedup_core_build = 0.0;
  double speedup_excluding_audit = 0.0;
  double speedup_excluding_diagnostics = 0.0;
  double speedup_excluding_marker = 0.0;
  double marker_time_fraction_of_wall = 0.0;
  double audit_time_fraction_of_wall = 0.0;
  double diagnostics_time_fraction_of_wall = 0.0;

  ContactBandDiagnostics diagnostics;
  ContactBandQualityMetrics quality;

  bool effective_speedup_claim_allowed = false;
  bool performance_claim_allowed = false;
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

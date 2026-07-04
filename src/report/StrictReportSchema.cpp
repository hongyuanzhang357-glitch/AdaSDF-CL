#include "adasdf/report/StrictReportSchema.h"

#include <algorithm>

namespace adasdf {

const char* strictReportSchemaVersion() {
  return "adasdf.strict_report.v1";
}

const std::vector<std::string>& strictReportRequiredFields() {
  static const std::vector<std::string> fields = {
      "schema_version",
      "adasdf_version",
      "git_commit",
      "tool_name",
      "case_id",
      "input_path",
      "input_sha256",
      "output_path",
      "output_sha256",
      "parameters",
      "status",
      "success",
      "failure_reason",
      "start_time_utc",
      "end_time_utc",
      "elapsed_ms",
      "platform",
      "cpu_threads",
      "cuda_enabled",
      "cuda_available"};
  return fields;
}

const std::vector<std::string>& strictReportOptionalMetricFields() {
  static const std::vector<std::string> fields = {
      "triangle_count",
      "vertex_count",
      "readiness_score",
      "build_time_ms",
      "memory_bytes",
      "compressed_memory_bytes",
      "compression_ratio",
      "max_abs_error",
      "mean_abs_error",
      "rms_error",
      "p95_error",
      "sign_mismatch_count",
      "near_surface_sign_mismatch_count",
      "query_time_ms",
      "benchmark_ns_per_query",
      "contact_count",
      "solver_contact_count",
      "broadphase_pair_count"};
  return fields;
}

const std::vector<std::string>& strictReportSummaryCsvHeader() {
  static const std::vector<std::string> header = [] {
    std::vector<std::string> values = strictReportRequiredFields();
    const auto& metrics = strictReportOptionalMetricFields();
    values.insert(values.end(), metrics.begin(), metrics.end());
    return values;
  }();
  return header;
}

bool isStrictReportRequiredField(const std::string& field) {
  const auto& fields = strictReportRequiredFields();
  return std::find(fields.begin(), fields.end(), field) != fields.end();
}

bool isStrictReportOptionalMetricField(const std::string& field) {
  const auto& fields = strictReportOptionalMetricFields();
  return std::find(fields.begin(), fields.end(), field) != fields.end();
}

}  // namespace adasdf


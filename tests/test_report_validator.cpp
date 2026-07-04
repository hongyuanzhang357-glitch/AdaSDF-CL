#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::RunManifest manifest =
      adasdf::makeRunManifest("validator", "case", {}, {});
  manifest.status = "ok";
  manifest.success = true;
  manifest.start_time_utc = "2026-07-04T00:00:00Z";
  manifest.end_time_utc = "2026-07-04T00:00:00Z";
  const std::string json = adasdf::StrictJsonWriter::toJson(manifest);
  if (!adasdf::ReportValidator::validateString(json).valid) {
    std::cerr << "valid strict report was rejected\n";
    return 1;
  }
  const std::string invalid = "{\"schema_version\":\"adasdf.strict_report.v1\"}";
  const adasdf::ReportValidationResult result =
      adasdf::ReportValidator::validateString(invalid);
  if (result.valid || result.errors.empty()) {
    std::cerr << "invalid strict report was accepted\n";
    return 1;
  }
  std::cout << "report validator passed\n";
  return 0;
}


#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::RunManifest manifest =
      adasdf::makeRunManifest("unit_tool", "case_json", "input.stl", "out.sdfbin");
  manifest.parameters["quoted"] = "a\"b";
  manifest.status = "ok";
  manifest.success = true;
  manifest.start_time_utc = "2026-07-04T00:00:00Z";
  manifest.end_time_utc = "2026-07-04T00:00:01Z";
  manifest.elapsed_ms = 1.25;
  manifest.metrics["triangle_count"] = 12.0;
  const std::string json = adasdf::StrictJsonWriter::toJson(manifest);
  if (json.find("\"schema_version\"") == std::string::npos ||
      json.find("\\\"") == std::string::npos) {
    std::cerr << "strict JSON writer did not emit expected fields\n";
    return 1;
  }
  const adasdf::ReportValidationResult validation =
      adasdf::ReportValidator::validateString(json);
  if (!validation.valid) {
    std::cerr << "strict JSON writer emitted invalid JSON\n";
    for (const std::string& error : validation.errors) {
      std::cerr << error << "\n";
    }
    return 1;
  }
  std::cout << "strict JSON writer passed\n";
  return 0;
}


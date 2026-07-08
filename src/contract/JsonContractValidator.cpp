#include "adasdf/contract/JsonContractValidator.h"

namespace adasdf {

JsonContractValidationResult JsonContractValidator::validateCommonFields(
    const std::string& json,
    const std::string& expected_schema_id) {
  JsonContractValidationResult result;
  const char* required[] = {
      "\"schema_id\"",
      "\"schema_version\"",
      "\"adasdf_version\"",
      "\"tool_name\"",
      "\"generated_at_utc\"",
      "\"status\"",
      "\"status_code\"",
      "\"success\"",
      "\"warnings\""};
  for (const char* field : required) {
    if (json.find(field) == std::string::npos) {
      result.errors.push_back(std::string("missing required field ") + field);
    }
  }
  if (!expected_schema_id.empty() &&
      json.find("\"schema_id\": \"" + expected_schema_id + "\"") ==
          std::string::npos) {
    result.errors.push_back("schema_id does not match " + expected_schema_id);
  }
  result.valid = result.errors.empty();
  return result;
}

}  // namespace adasdf

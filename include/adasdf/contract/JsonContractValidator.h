#pragma once

#include <string>
#include <vector>

namespace adasdf {

struct JsonContractValidationResult {
  bool valid = false;
  std::vector<std::string> errors;
};

class JsonContractValidator {
 public:
  static JsonContractValidationResult validateCommonFields(
      const std::string& json,
      const std::string& expected_schema_id);
};

}  // namespace adasdf

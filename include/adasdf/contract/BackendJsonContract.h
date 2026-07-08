#pragma once

#include <string>
#include <utility>
#include <vector>

#include "adasdf/contract/ContractVersion.h"
#include "adasdf/contract/ErrorCode.h"
#include "adasdf/contract/Status.h"
#include "adasdf/contract/Warning.h"
#include "adasdf/version.h"

namespace adasdf {

struct BackendJsonContract {
  std::string schema_id;
  int schema_version = backendJsonContractVersion();
  std::string adasdf_version = versionString();
  std::string tool_name;
  std::string generated_at_utc;
  JsonStatus status = JsonStatus::Ok;
  ErrorCode status_code = ErrorCode::OK;
  bool success = true;
  std::vector<Warning> warnings;
  std::vector<std::pair<std::string, std::string>> payload_fields;
};

}  // namespace adasdf

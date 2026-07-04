#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "adasdf/report/RunManifest.h"

namespace adasdf {

struct CaseManifest {
  std::string case_id;
  std::string tool_name;
  std::filesystem::path input_path;
  std::filesystem::path output_path;
  std::map<std::string, std::string> parameters;
};

RunManifest runManifestFromCase(const CaseManifest& case_manifest);

}  // namespace adasdf


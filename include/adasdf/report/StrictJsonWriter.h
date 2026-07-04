#pragma once

#include <filesystem>
#include <string>

#include "adasdf/report/RunManifest.h"

namespace adasdf {

class StrictJsonWriter {
 public:
  static std::string escape(const std::string& value);
  static std::string toJson(const RunManifest& manifest);
  static bool writeFile(
      const std::filesystem::path& path,
      const RunManifest& manifest,
      std::string* error_message = nullptr);
};

}  // namespace adasdf


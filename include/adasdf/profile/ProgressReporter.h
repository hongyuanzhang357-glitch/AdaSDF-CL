#pragma once

#include <filesystem>
#include <string>

namespace adasdf {

class ProgressReporter {
 public:
  ProgressReporter() = default;
  ProgressReporter(
      std::string tool_name,
      bool stderr_enabled,
      std::filesystem::path jsonl_path);

  bool enabled() const;
  void emit(
      const std::string& stage,
      const std::string& message,
      std::size_t current = 0,
      std::size_t total = 0) const;

 private:
  std::string tool_name_;
  bool stderr_enabled_ = false;
  std::filesystem::path jsonl_path_;
};

}  // namespace adasdf

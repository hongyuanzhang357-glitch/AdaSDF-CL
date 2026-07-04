#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef ADASDF_CL_VALIDATE_REPORT
#define ADASDF_CL_VALIDATE_REPORT ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos && tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos && tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

bool exitCodeIsTwo(int code) {
  return code == 2 || code == (2 << 8);
}

}  // namespace

int main() {
  const std::string validate_tool = ADASDF_CL_VALIDATE_REPORT;
  if (validate_tool.empty()) {
    std::cout << "SKIP: validate report CLI was not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto invalid = temp / "invalid_strict_report.json";
  {
    std::ofstream file(invalid);
    file << "{\"schema_version\":\"adasdf.strict_report.v1\"}\n";
  }
  const int code = std::system(
      (executableCommand(validate_tool) + " " + quote(invalid)).c_str());
  if (!exitCodeIsTwo(code)) {
    std::cerr << "invalid report did not return schema-invalid status\n";
    return 1;
  }
  std::cout << "validate report CLI passed\n";
  return 0;
}


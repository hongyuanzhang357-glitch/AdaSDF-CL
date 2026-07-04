#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_WRITE_MANIFEST
#define ADASDF_CL_WRITE_MANIFEST ""
#endif
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

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  const std::string write_tool = ADASDF_CL_WRITE_MANIFEST;
  const std::string validate_tool = ADASDF_CL_VALIDATE_REPORT;
  if (write_tool.empty() || validate_tool.empty()) {
    std::cout << "SKIP: strict report CLI tools were not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto manifest = temp / "write_manifest_cli.json";
  const std::string command =
      executableCommand(write_tool) +
      " --case-id case001 --tool build_compressed_sdf --input model.stl "
      "--output model.sdfbin --param target_error=1e-3 --out " +
      quote(manifest);
  if (std::system(command.c_str()) != 0 || !std::filesystem::exists(manifest)) {
    std::cerr << "write manifest CLI failed\n";
    return 1;
  }
  if (readFile(manifest).find("\"case_id\": \"case001\"") ==
      std::string::npos) {
    std::cerr << "manifest did not contain case id\n";
    return 1;
  }
  if (std::system((executableCommand(validate_tool) + " " +
                   quote(manifest)).c_str()) != 0) {
    std::cerr << "validate report failed for generated manifest\n";
    return 1;
  }
  std::cout << "write manifest CLI passed\n";
  return 0;
}


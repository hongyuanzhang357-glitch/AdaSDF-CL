#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_VALIDATE_REPORT
#define ADASDF_CL_VALIDATE_REPORT ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
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
  const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  const std::string validate_tool = ADASDF_CL_VALIDATE_REPORT;
  if (build_tool.empty() || validate_tool.empty()) {
    std::cout << "SKIP: strict JSON core CLI tools were not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const std::filesystem::path fixture_dir =
      ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
  const auto output = temp / "strict_core_cli.sdfbin";
  const auto strict_json = temp / "strict_core_cli_report.json";
  const auto stdout_txt = temp / "strict_core_cli_stdout.txt";
  const std::string command =
      executableCommand(build_tool) + " " +
      quote(fixture_dir / "closed_cube_ascii.stl") + " " + quote(output) +
      " --target-error 1e-3 --max-level 2 --block-resolution 5 --max-rank 5 "
      "--strict-json " + quote(strict_json) + " --case-id strict_core > " +
      quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      !std::filesystem::exists(strict_json)) {
    std::cerr << "build_compressed_sdf --strict-json failed\n";
    return 1;
  }
  if (readFile(strict_json).find("\"case_id\": \"strict_core\"") ==
      std::string::npos) {
    std::cerr << "strict JSON report missing case id\n";
    return 1;
  }
  if (std::system((executableCommand(validate_tool) + " " +
                   quote(strict_json)).c_str()) != 0) {
    std::cerr << "validate_report rejected core CLI strict JSON\n";
    return 1;
  }
  std::cout << "strict JSON core CLI passed\n";
  return 0;
}


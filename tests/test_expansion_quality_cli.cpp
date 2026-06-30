#include <adasdf/adasdf.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ADASDF_CL_EXPANSION_QUALITY
#define ADASDF_CL_EXPANSION_QUALITY ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  return tool;
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
  return "\"" + tool + "\"";
#endif
}

}  // namespace

int main() {
  const std::string tool = ADASDF_CL_EXPANSION_QUALITY;
  if (tool.empty()) {
    std::cout << "SKIP: adasdf_expansion_quality was not built\n";
    return 0;
  }

  std::filesystem::path dir = ADASDF_CL_TEST_TEMP_DIR;
  if (dir.empty()) {
    dir = std::filesystem::temp_directory_path();
  }
  std::filesystem::create_directories(dir);
  const std::filesystem::path model_path = dir / "quality_cli_demo.sdfbin";
  const std::filesystem::path csv_path = dir / "quality_cli.csv";

  adasdf::DemoAdaptiveBuildRequest request;
  request.use_surrogate = false;
  const auto build = adasdf::DemoAdaptiveSDFBuilder::build(request);
  adasdf::SDFBinWriter::write(model_path.string(), *build.model);

  const int usage_code = std::system(executableCommand(tool).c_str());
  if (usage_code != 0) {
    std::cerr << "no-arg CLI usage returned failure\n";
    return 1;
  }

  const std::string command =
      executableCommand(tool) + " \"" + model_path.string() +
      "\" --expansion global --global-resolution 16 --samples 128 --out \"" +
      csv_path.string() + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    std::cerr << "quality CLI command failed\n";
    return 1;
  }

  const std::string csv = readFile(csv_path);
  const std::vector<std::string> required = {
      "max_abs_error",
      "mean_abs_error",
      "rms_error",
      "p95_abs_error",
      "sign_mismatch_count",
      "sign_mismatch_rate",
      "ambiguous_sign_count",
      "ambiguous_sign_rate",
      "near_surface_sign_mismatch_count",
      "near_surface_sign_mismatch_rate",
      "fallback_count",
      "fallback_rate",
      "worst_point_id"};
  for (const std::string& field : required) {
    if (!contains(csv, field)) {
      std::cerr << "missing quality CLI CSV field: " << field << "\n";
      return 1;
    }
  }

  std::cout << "expansion quality CLI passed\n";
  return 0;
}

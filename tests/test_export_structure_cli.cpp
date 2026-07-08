#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_EXPORT_STRUCTURE
#define ADASDF_CL_EXPORT_STRUCTURE ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BUILD_COMPRESSED_SDF).empty() ||
      std::string(ADASDF_CL_EXPORT_STRUCTURE).empty()) {
    std::cout << "SKIP: required tools not built\n";
    return 0;
  }
  const std::filesystem::path temp = std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "backend_contract_structure";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube.sdfbin";
  const auto json = temp / "structure.json";
  const auto fixture = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / "closed_cube_ascii.stl";
  if (runCommand(executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
                 quotePath(fixture) + " " + quotePath(model) +
                 " --max-level 2 --block-resolution 5 > " +
                 quotePath(temp / "build.txt")) != 0) {
    return 1;
  }
  if (runCommand(executableCommand(ADASDF_CL_EXPORT_STRUCTURE) + " " +
                 quotePath(model) + " --json > " + quotePath(json)) != 0) {
    return 1;
  }
  const std::string text = readText(json);
  if (!containsText(text, "\"schema_id\": \"adasdf.structure.v1\"") ||
      !containsText(text, "\"blocks\"") ||
      !containsText(text, "\"levels\"")) {
    std::cerr << "structure JSON contract missing expected fields\n";
    return 1;
  }
  return 0;
}

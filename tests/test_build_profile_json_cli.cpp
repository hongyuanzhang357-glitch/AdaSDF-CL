#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BUILD_COMPRESSED_SDF).empty()) {
    std::cout << "SKIP: compressed builder not built\n";
    return 0;
  }
  const std::filesystem::path temp = std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "build_profile_json";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube.sdfbin";
  const auto profile = temp / "profile.json";
  const auto progress = temp / "progress.jsonl";
  const auto fixture = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / "closed_cube_ascii.stl";
  if (runCommand(executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
                 quotePath(fixture) + " " + quotePath(model) +
                 " --max-level 2 --block-resolution 5 --profile-json " +
                 quotePath(profile) + " --progress-json " +
                 quotePath(progress) + " > " + quotePath(temp / "build.txt")) != 0) {
    return 1;
  }
  const std::string profile_text = readText(profile);
  const std::string progress_text = readText(progress);
  if (!containsText(profile_text, "\"schema_id\": \"adasdf.build_profile.v1\"") ||
      !containsText(profile_text, "\"profile_status\": \"completed\"") ||
      !containsText(profile_text, "\"timings\"") ||
      !containsText(progress_text, "\"schema_id\":\"adasdf.progress.v1\"")) {
    std::cerr << "build profile/progress JSON missing expected fields\n";
    return 1;
  }
  return 0;
}

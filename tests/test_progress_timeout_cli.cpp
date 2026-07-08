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
  const std::filesystem::path temp = std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "progress_timeout";
  std::filesystem::create_directories(temp);
  const auto model = temp / "timeout.sdfbin";
  const auto profile = temp / "timeout_profile.json";
  const auto fixture = std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) / "closed_cube_ascii.stl";
  const int rc = runCommand(executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
                            quotePath(fixture) + " " + quotePath(model) +
                            " --profile-json " + quotePath(profile) +
                            " --max-seconds 0.001 > " +
                            quotePath(temp / "timeout.txt"));
  if (rc == 0 || !std::filesystem::exists(profile)) {
    std::cerr << "timeout run should fail and write profile\n";
    return 1;
  }
  const std::string text = readText(profile);
  if (!containsText(text, "\"profile_status\": \"timeout\"") ||
      !containsText(text, "ADASDF_TIMEOUT") ||
      std::filesystem::exists(model)) {
    std::cerr << "timeout profile/model state incorrect\n";
    return 1;
  }
  return 0;
}

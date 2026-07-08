#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_BUILD_ADAPTIVE_SDF
#define ADASDF_CL_BUILD_ADAPTIVE_SDF ""
#endif
#ifndef ADASDF_CL_COMPRESS_ADAPTIVE_SDF
#define ADASDF_CL_COMPRESS_ADAPTIVE_SDF ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

bool checkTimeoutProfile(
    const std::string& tool,
    const std::string& stem,
    const std::filesystem::path& fixture,
    const std::filesystem::path& temp) {
  const auto model = temp / (stem + ".sdfbin");
  const auto profile = temp / (stem + "_profile.json");
  const auto progress = temp / (stem + "_progress.jsonl");
  const auto output = temp / (stem + "_stdout.txt");
  const int rc = runCommand(
      executableCommand(tool) + " " + quotePath(fixture) + " " +
      quotePath(model) + " --profile-json " + quotePath(profile) +
      " --progress-json " + quotePath(progress) +
      " --max-seconds 0.001 --distance-backend bvh --threads auto > " +
      quotePath(output) + " 2>&1");
  if (rc == 0) {
    std::cerr << stem << " timeout run unexpectedly succeeded\n";
    return false;
  }
  if (!std::filesystem::exists(profile) ||
      !std::filesystem::exists(progress)) {
    std::cerr << stem << " did not write timeout profile/progress\n";
    return false;
  }
  const std::string profile_text = readText(profile);
  const std::string progress_text = readText(progress);
  if (!containsText(profile_text, "\"schema_id\": \"adasdf.build_profile.v1\"") ||
      !containsText(profile_text, "\"profile_status\": \"timeout\"") ||
      !containsText(profile_text, "ADASDF_TIMEOUT") ||
      !containsText(progress_text, "adasdf.progress.v1") ||
      std::filesystem::exists(model)) {
    std::cerr << stem << " timeout contract fields are incorrect\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  if (std::string(ADASDF_CL_BUILD_DENSE_SDF).empty() ||
      std::string(ADASDF_CL_BUILD_ADAPTIVE_SDF).empty() ||
      std::string(ADASDF_CL_COMPRESS_ADAPTIVE_SDF).empty()) {
    std::cout << "SKIP: build CLIs not built\n";
    return 0;
  }

  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "build_cli_profile";
  std::filesystem::create_directories(temp);

  if (!checkTimeoutProfile(
          ADASDF_CL_BUILD_DENSE_SDF,
          "dense",
          fixture,
          temp)) {
    return 1;
  }
  if (!checkTimeoutProfile(
          ADASDF_CL_BUILD_ADAPTIVE_SDF,
          "adaptive",
          fixture,
          temp)) {
    return 1;
  }
  if (!checkTimeoutProfile(
          ADASDF_CL_COMPRESS_ADAPTIVE_SDF,
          "compress",
          fixture,
          temp)) {
    return 1;
  }
  return 0;
}

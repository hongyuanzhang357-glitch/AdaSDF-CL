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
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "build_cache_cli";
  std::filesystem::create_directories(temp);
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto model = temp / "cube_cached.sdfbin";
  const auto profile = temp / "cube_cached_profile.json";
  const auto report = temp / "cube_cached_report.md";
  const auto stdout_txt = temp / "stdout.txt";
  const std::string command =
      executableCommand(ADASDF_CL_BUILD_COMPRESSED_SDF) + " " +
      quotePath(fixture) + " " + quotePath(model) +
      " --target-error 1e-3 --max-level 1 --block-resolution 4 "
      "--max-rank 4 --sampling contact-band --contact-band-width 1e-3 "
      "--contact-band-marker distance-aware --marker-cell-size-factor 0.5 "
      "--marker-safety-factor 1.0 --local-halo-only --sample-cache block "
      "--corner-cache block --sign-cache on --distance-cache on "
      "--marker-cache on --report-cache-stats --distance-backend bvh "
      "--threads 1 --profile-json " +
      quotePath(profile) + " --report " + quotePath(report) + " > " +
      quotePath(stdout_txt) + " 2>&1";
  if (runCommand(command) != 0 || !std::filesystem::exists(model) ||
      !std::filesystem::exists(profile) ||
      !std::filesystem::exists(report)) {
    std::cerr << "build cache CLI failed\n";
    return 1;
  }
  const std::string stdout_text = readText(stdout_txt);
  const std::string profile_text = readText(profile);
  if (!containsText(stdout_text, "Sample cache") ||
      !containsText(profile_text, "\"sample_cache_enabled\"") ||
      !containsText(profile_text, "\"sample_cache_hits\"") ||
      !containsText(profile_text, "\"distance_queries_saved\"") ||
      !containsText(profile_text, "\"marker_cache_time_ms\"")) {
    std::cerr << "build cache CLI output/profile missing cache fields\n";
    return 1;
  }
  return 0;
}

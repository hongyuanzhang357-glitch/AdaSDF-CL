#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_BUILD_NARROWBAND_BRICK_SDF
#define ADASDF_CL_BUILD_NARROWBAND_BRICK_SDF ""
#endif
#ifndef ADASDF_CL_INFO
#define ADASDF_CL_INFO ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_BUILD_NARROWBAND_BRICK_SDF).empty() ||
      std::string(ADASDF_CL_INFO).empty()) {
    std::cout << "SKIP: narrow-band brick SDF CLI tools were not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "build_narrowband_brick_sdf_cli";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube_nb_brick.sdfbin";
  const auto profile = temp / "profile.json";
  const auto progress = temp / "progress.jsonl";
  const auto report = temp / "report.md";
  const auto stdout_txt = temp / "stdout.txt";
  const auto info_txt = temp / "info.txt";

  const int build_rc = runCommand(
      executableCommand(ADASDF_CL_BUILD_NARROWBAND_BRICK_SDF) + " " +
      quotePath(fixture) + " " + quotePath(model) +
      " --max-sampling-level 3 --brick-level-map 0-2:0,3-5:1 "
      "--brick-min-tensor-dim 8 --brick-target-tensor-dim 8 "
      "--brick-max-tensor-dim 8 "
      "--sampling-levels-per-brick-level 3 "
      "--sampling-refine-zero-crossing --sampling-refine-contact-band "
      "--sampling-contact-band-width 0.05 "
      "--tensor-fill contact-exact-far-interp "
      "--zero-crossing-exact-stencil 2 "
      "--compression-mode existing-low-rank --max-rank 4 "
      "--profile-json " +
      quotePath(profile) + " --progress-json " + quotePath(progress) +
      " --report " + quotePath(report) + " > " + quotePath(stdout_txt));
  if (build_rc != 0 || !std::filesystem::exists(model) ||
      !std::filesystem::exists(profile) ||
      !std::filesystem::exists(progress) ||
      !std::filesystem::exists(report)) {
    std::cerr << "narrow-band brick build CLI failed\n";
    return 1;
  }
  const std::string profile_text = readText(profile);
  const std::string report_text = readText(report);
  const std::string stdout_text = readText(stdout_txt);
  if (!containsText(profile_text, "adasdf.narrowband_brick_build_profile.v1") ||
      !containsText(profile_text, "decoupled-sampling-octree-compression-brick") ||
      !containsText(profile_text, "brick_count_by_level") ||
      !containsText(report_text, "Narrow-Band Brick SDF Build") ||
      !containsText(stdout_text, "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")) {
    std::cerr << "narrow-band brick build outputs missing expected fields\n";
    return 1;
  }
  const int info_rc = runCommand(
      executableCommand(ADASDF_CL_INFO) + " " + quotePath(model) +
      " > " + quotePath(info_txt));
  if (info_rc != 0 ||
      !containsText(readText(info_txt), "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")) {
    std::cerr << "adasdf_info failed on narrow-band brick output\n";
    return 1;
  }
  std::cout << "narrow-band brick SDF CLI passed\n";
  return 0;
}

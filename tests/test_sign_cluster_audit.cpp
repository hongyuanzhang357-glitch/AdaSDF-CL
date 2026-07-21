#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_AUDIT_SDF_ACCURACY
#define ADASDF_CL_AUDIT_SDF_ACCURACY ""
#endif
#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_AUDIT_SDF_ACCURACY).empty() ||
      std::string(ADASDF_CL_BUILD_DENSE_SDF).empty()) {
    std::cout << "SKIP: audit cluster CLI tools were not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "sign_cluster_audit";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube_dense.sdfbin";
  const auto json = temp / "cluster.json";
  const auto csv = temp / "cluster.csv";
  const auto dump = temp / "mismatch.csv";
  const int build_rc = runCommand(
      executableCommand(ADASDF_CL_BUILD_DENSE_SDF) + " " +
      quotePath(fixture) + " " + quotePath(model) +
      " --resolution 16 --padding 0.05 --accel bvh > " +
      quotePath(temp / "build_stdout.txt"));
  if (build_rc != 0 || !std::filesystem::exists(model)) {
    std::cerr << "dense SDF fixture build failed\n";
    return 1;
  }
  const int audit_rc = runCommand(
      executableCommand(ADASDF_CL_AUDIT_SDF_ACCURACY) +
      " --sdf " + quotePath(model) + " --stl " + quotePath(fixture) +
      " --mode near-surface --surface-samples 8 "
      "--offsets -0.01,0.01 --near-band 0.05 --normal-audit "
      "--cluster-sign-errors --offset-bin-report --block-source-report "
      "--query-source-report --dump-mismatch-samples " +
      quotePath(dump) + " --max-dump-samples 8 --json " +
      quotePath(json) + " --csv " + quotePath(csv) +
      " --case-id cube_cluster");
  if (audit_rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(csv) || !std::filesystem::exists(dump)) {
    std::cerr << "cluster audit CLI failed\n";
    return 1;
  }
  const std::string json_text = readText(json);
  const std::string csv_text = readText(csv);
  if (!containsText(json_text, "offset_bins") ||
      !containsText(json_text, "block_level_bins") ||
      !containsText(csv_text, "corner_sign_pattern") ||
      !containsText(readText(dump), "false_inside_or_false_outside")) {
    std::cerr << "cluster audit output missing fields\n";
    return 1;
  }
  std::cout << "sign cluster audit passed\n";
  return 0;
}

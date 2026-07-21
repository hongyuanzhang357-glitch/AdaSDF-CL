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
    std::cout << "SKIP: audit SDF accuracy CLI tools were not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "audit_sdf_accuracy_cli";
  std::filesystem::create_directories(temp);
  const auto model = temp / "cube_dense.sdfbin";
  const auto json = temp / "audit.json";
  const auto csv = temp / "audit.csv";
  const auto report = temp / "audit.md";
  const auto stdout_txt = temp / "audit_stdout.txt";

  const int build_rc = runCommand(
      executableCommand(ADASDF_CL_BUILD_DENSE_SDF) + " " +
      quotePath(fixture) + " " + quotePath(model) +
      " --resolution 24 --padding 0.05 --accel bvh > " +
      quotePath(temp / "build_stdout.txt"));
  if (build_rc != 0 || !std::filesystem::exists(model)) {
    std::cerr << "dense SDF fixture build failed\n";
    return 1;
  }

  const int audit_rc = runCommand(
      executableCommand(ADASDF_CL_AUDIT_SDF_ACCURACY) +
      " --sdf " + quotePath(model) + " --stl " + quotePath(fixture) +
      " --mode near-surface --surface-samples 8 "
      "--offsets -0.01,0,0.01 --near-band 0.05 --normal-audit "
      "--normal-eps auto --json " +
      quotePath(json) + " --csv " + quotePath(csv) + " --report " +
      quotePath(report) + " --case-id cube_cli_audit > " +
      quotePath(stdout_txt));
  if (audit_rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(csv) || !std::filesystem::exists(report)) {
    std::cerr << "audit SDF accuracy CLI failed\n";
    return 1;
  }

  const std::string json_text = readText(json);
  const std::string report_text = readText(report);
  const std::string stdout_text = readText(stdout_txt);
  if (!containsText(json_text, "adasdf.sdf_accuracy_audit.v1") ||
      !containsText(json_text, "near_surface_sample_count") ||
      !containsText(json_text, "full_quality_passed") ||
      !containsText(report_text, "SDF Near-Surface Accuracy Audit") ||
      !containsText(stdout_text, "SDF accuracy audit completed")) {
    std::cerr << "audit SDF accuracy CLI output missing fields\n";
    return 1;
  }
  std::cout << "audit SDF accuracy CLI passed\n";
  return 0;
}

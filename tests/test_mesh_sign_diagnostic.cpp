#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_DIAGNOSE_MESH_SIGN
#define ADASDF_CL_DIAGNOSE_MESH_SIGN ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_DIAGNOSE_MESH_SIGN).empty()) {
    std::cout << "SKIP: mesh sign diagnostic CLI was not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "mesh_sign_diagnostic";
  std::filesystem::create_directories(temp);
  const auto json = temp / "mesh_sign.json";
  const auto report = temp / "mesh_sign.md";
  const auto stdout_txt = temp / "mesh_sign_stdout.txt";
  const int rc = runCommand(
      executableCommand(ADASDF_CL_DIAGNOSE_MESH_SIGN) + " " +
      quotePath(fixture) +
      " --samples 12 --offsets -0.01,0.01 --json " +
      quotePath(json) + " --report " + quotePath(report) +
      " --case-id cube_mesh_sign > " + quotePath(stdout_txt));
  if (rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(report)) {
    std::cerr << "mesh sign diagnostic CLI failed\n";
    return 1;
  }
  const std::string json_text = readText(json);
  if (!containsText(json_text, "adasdf.mesh_sign_diagnostic.v1") ||
      !containsText(json_text, "ray_x_y_z_disagreement_count") ||
      !containsText(readText(stdout_txt), "mesh sign diagnostic completed")) {
    std::cerr << "mesh sign diagnostic output missing fields\n";
    return 1;
  }
  std::cout << "mesh sign diagnostic passed\n";
  return 0;
}

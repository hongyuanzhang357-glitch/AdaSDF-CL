#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_DIAGNOSE_ADAPTIVE_TREE
#define ADASDF_CL_DIAGNOSE_ADAPTIVE_TREE ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_DIAGNOSE_ADAPTIVE_TREE).empty()) {
    std::cout << "SKIP: diagnose adaptive tree tool was not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "diagnose_adaptive_tree_cli";
  std::filesystem::create_directories(temp);
  const auto json = temp / "stats.json";
  const auto report = temp / "stats.md";
  const auto out = temp / "stdout.txt";

  const int rc = runCommand(
      executableCommand(ADASDF_CL_DIAGNOSE_ADAPTIVE_TREE) + " " +
      quotePath(fixture) +
      " --max-level 2 --block-resolution 4 "
      "--adaptive-leaf-mode mixed --sampling contact-band "
      "--contact-band-width 0.02 --json " +
      quotePath(json) + " --report " + quotePath(report) +
      " > " + quotePath(out));
  if (rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(report)) {
    std::cerr << "diagnose adaptive tree CLI failed\n";
    return 1;
  }
  const std::string json_text = readText(json);
  const std::string report_text = readText(report);
  const std::string stdout_text = readText(out);
  if (!containsText(json_text, "adasdf.adaptive_tree_stats.v1") ||
      !containsText(json_text, "leaf_block_count") ||
      !containsText(json_text, "uniform_max_level_logical_node_count") ||
      !containsText(report_text, "Adaptive Tree Diagnostics") ||
      !containsText(stdout_text, "Sparsity ratio vs uniform max-level")) {
    std::cerr << "diagnose adaptive tree CLI output missing fields\n";
    return 1;
  }
  std::cout << "diagnose adaptive tree CLI passed\n";
  return 0;
}

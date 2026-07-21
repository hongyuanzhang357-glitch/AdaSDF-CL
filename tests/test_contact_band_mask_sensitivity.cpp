#include <filesystem>
#include <iostream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_DIAGNOSE_CONTACT_BAND_MASK
#define ADASDF_CL_DIAGNOSE_CONTACT_BAND_MASK ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  if (std::string(ADASDF_CL_DIAGNOSE_CONTACT_BAND_MASK).empty()) {
    std::cout << "SKIP: contact-band mask diagnostic CLI was not built\n";
    return 0;
  }
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto temp =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "contact_band_mask";
  std::filesystem::create_directories(temp);
  const auto json = temp / "mask.json";
  const auto csv = temp / "mask.csv";
  const auto report = temp / "mask.md";
  const int rc = runCommand(
      executableCommand(ADASDF_CL_DIAGNOSE_CONTACT_BAND_MASK) + " " +
      quotePath(fixture) +
      " --max-level 2 --block-resolution 5 --adaptive-leaf-mode mixed "
      "--contact-band-widths 0.001,0.05 "
      "--halo-exact-layers-list 1 "
      "--marker-safety-factors 1.0 "
      "--contact-band-marker distance-aware "
      "--json " +
      quotePath(json) + " --csv " + quotePath(csv) + " --report " +
      quotePath(report));
  if (rc != 0 || !std::filesystem::exists(json) ||
      !std::filesystem::exists(csv) || !std::filesystem::exists(report)) {
    std::cerr << "contact-band mask sensitivity CLI failed\n";
    return 1;
  }
  const std::string json_text = readText(json);
  if (!containsText(json_text, "adasdf.contact_band_mask_sensitivity.v1") ||
      !containsText(json_text, "exact_node_count") ||
      !containsText(json_text, "mask_changed_vs_baseline")) {
    std::cerr << "contact-band mask output missing fields\n";
    return 1;
  }
  std::cout << "contact-band mask sensitivity passed\n";
  return 0;
}

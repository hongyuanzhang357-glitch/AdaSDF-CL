#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "contract_cli_test_helpers.h"

#ifndef ADASDF_CL_CREATE_SDF
#define ADASDF_CL_CREATE_SDF "adasdf_create_sdf"
#endif

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) / "create_sdf_cli";
  std::filesystem::create_directories(dir);
  const std::filesystem::path input =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const std::filesystem::path output = dir / "cube.sdfbin";
  const std::filesystem::path report = dir / "report.json";
  const std::filesystem::path csv = dir / "report.csv";
  std::ostringstream command;
  command << executableCommand(ADASDF_CL_CREATE_SDF) << " "
          << quotePath(input) << " " << quotePath(output)
          << " --max-sdf-file-bytes 67108864"
          << " --max-decoded-block-bytes 262144"
          << " --max-zero-surface-abs-error 0.15"
          << " --report " << quotePath(report)
          << " --csv " << quotePath(csv);
  const int code = std::system(command.str().c_str());
  if (code != 0 || !std::filesystem::exists(output) ||
      !std::filesystem::exists(report)) {
    std::cerr << "adasdf_create_sdf failed\n";
    return 1;
  }
  if (!std::filesystem::exists(csv) ||
      readText(csv).find("sdf_file_bytes_actual") == std::string::npos) {
    std::cerr << "create CSV report is missing stable fields\n";
    return 1;
  }
  std::ifstream input_report(report);
  const std::string json(
      (std::istreambuf_iterator<char>(input_report)),
      std::istreambuf_iterator<char>());
  if (json.find("\"status\":\"Feasible\"") == std::string::npos ||
      json.find("\"max_sdf_file_bytes\"") == std::string::npos ||
      json.find("\"max_decoded_block_bytes\"") == std::string::npos ||
      json.find("\"max_zero_surface_abs_error\"") == std::string::npos) {
    std::cerr << "create report is missing hard-constraint fields\n";
    return 1;
  }
  input_report.close();
  std::filesystem::remove_all(dir);
  return 0;
}

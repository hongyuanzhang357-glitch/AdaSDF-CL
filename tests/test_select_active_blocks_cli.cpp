#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_SELECT_ACTIVE_BLOCKS
#define ADASDF_CL_SELECT_ACTIVE_BLOCKS ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos && tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos && tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  const std::string select_tool = ADASDF_CL_SELECT_ACTIVE_BLOCKS;
  if (build_tool.empty() || select_tool.empty()) {
    std::cout << "SKIP: active block selection CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "active_select_cli_compressed.sdfbin";
  const auto stdout_txt = temp / "active_select_cli_stdout.txt";
  const auto out_csv = temp / "active_select_cli_blocks.csv";
  const auto report = temp / "active_select_cli_report.md";
  const auto json = temp / "active_select_cli_report.json";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) / "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";

  const std::string build_cmd = executableCommand(build_tool);
  const std::string select_cmd = executableCommand(select_tool);
  if (std::system((build_cmd + " " + quote(fixture) + " " + quote(model) +
                   " --max-level 1 --block-resolution 4 --unsigned --fixed-rank 2"
                   " > " + quote(temp / "active_select_build.txt"))
                      .c_str()) != 0) {
    std::cerr << "failed to build compressed model\n";
    return 1;
  }

  const std::string command =
      select_cmd + " " + quote(model) + " " + quote(samples) +
      " --threshold 1.0 --selection-band 0.1 --extra-margin 0.02 "
      "--out " + quote(out_csv) +
      " --report " + quote(report) +
      " --json " + quote(json) +
      " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      !std::filesystem::exists(out_csv) ||
      !std::filesystem::exists(report) ||
      !std::filesystem::exists(json) ||
      !contains(readFile(stdout_txt), "Status: ok") ||
      !contains(readFile(out_csv), "block_id")) {
    std::cerr << "active block selection CLI failed\n";
    return 1;
  }
  std::cout << "active block selection CLI passed\n";
  return 0;
}

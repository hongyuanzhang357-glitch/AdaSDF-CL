#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_CONTACT_CANDIDATES
#define ADASDF_CL_CONTACT_CANDIDATES ""
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

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_BUILD_DENSE_SDF;
  const std::string candidates_tool = ADASDF_CL_CONTACT_CANDIDATES;
  if (build_tool.empty() || candidates_tool.empty()) {
    std::cout << "SKIP: contact candidate CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "contact_candidates_cli_dense.sdfbin";
  const auto stdout_txt = temp / "contact_candidates_cli_stdout.txt";
  const auto out_csv = temp / "contact_candidates_cli.csv";
  const auto report = temp / "contact_candidates_cli.md";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_contact_candidates.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const std::string build_cmd = executableCommand(build_tool);
  const std::string candidates_cmd = executableCommand(candidates_tool);
  if (std::system((build_cmd + " " + quote(fixture) + " " + quote(model) +
                   " --resolution 24 > " + quote(temp / "candidates_build.txt"))
                      .c_str()) != 0) {
    return 1;
  }
  const std::string command =
      candidates_cmd + " " + quote(model) + " " + quote(samples) +
      " --top-k 4 --threshold 1e-3 --reduction-radius 0.05 --with-normal "
      "--out " + quote(out_csv) + " --report " + quote(report) + " > " +
      quote(stdout_txt);
  if (std::system(command.c_str()) != 0 || !std::filesystem::exists(out_csv) ||
      !std::filesystem::exists(report) ||
      readFile(stdout_txt).find("Reduced candidates") == std::string::npos ||
      readFile(out_csv).find("rank") == std::string::npos) {
    std::cerr << "contact candidates CLI failed\n";
    return 1;
  }
  std::cout << "contact candidates CLI passed\n";
  return 0;
}

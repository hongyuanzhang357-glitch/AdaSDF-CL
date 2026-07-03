#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_SOLVER_CONTACT_CANDIDATES
#define ADASDF_CL_SOLVER_CONTACT_CANDIDATES ""
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
  const std::string solver_tool = ADASDF_CL_SOLVER_CONTACT_CANDIDATES;
  if (build_tool.empty() || solver_tool.empty()) {
    std::cout << "SKIP: solver contact candidate CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "solver_contacts_cli_dense.sdfbin";
  const auto out_csv = temp / "solver_contacts_cli.csv";
  const auto raw_csv = temp / "solver_contacts_raw_candidates.csv";
  const auto report = temp / "solver_contacts_cli.md";
  const auto stdout_txt = temp / "solver_contacts_cli_stdout.txt";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_contact_candidates.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  if (std::system((executableCommand(build_tool) + " " + quote(fixture) + " " +
                   quote(model) + " --resolution 24 > " +
                   quote(temp / "solver_contacts_build.txt"))
                      .c_str()) != 0) {
    return 1;
  }
  const std::string command =
      executableCommand(solver_tool) + " " + quote(model) + " " + quote(samples) +
      " --threshold 1e-3 --top-k 32 --max-contacts 4 --patch-radius 0.05 "
      "--with-normal --out " + quote(out_csv) +
      " --candidates-out " + quote(raw_csv) +
      " --report " + quote(report) + " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      !std::filesystem::exists(out_csv) ||
      !std::filesystem::exists(raw_csv) ||
      !std::filesystem::exists(report) ||
      readFile(stdout_txt).find("Solver contacts") == std::string::npos ||
      readFile(out_csv).find("stable_key") == std::string::npos) {
    std::cerr << "solver contact candidates CLI failed\n";
    return 1;
  }
  std::cout << "solver contact candidates CLI passed\n";
  return 0;
}

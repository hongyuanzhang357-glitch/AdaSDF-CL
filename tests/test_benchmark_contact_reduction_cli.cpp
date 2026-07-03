#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_BENCHMARK_CONTACT_REDUCTION
#define ADASDF_CL_BENCHMARK_CONTACT_REDUCTION ""
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
  const std::string bench_tool = ADASDF_CL_BENCHMARK_CONTACT_REDUCTION;
  if (build_tool.empty() || bench_tool.empty()) {
    std::cout << "SKIP: contact reduction benchmark CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "contact_reduction_cli_dense.sdfbin";
  const auto csv = temp / "contact_reduction_cli.csv";
  const auto stdout_txt = temp / "contact_reduction_cli_stdout.txt";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_contact_candidates.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  if (std::system((executableCommand(build_tool) + " " + quote(fixture) + " " +
                   quote(model) + " --resolution 24 > " +
                   quote(temp / "contact_reduction_build.txt"))
                      .c_str()) != 0) {
    return 1;
  }
  const std::string command =
      executableCommand(bench_tool) + " " + quote(model) + " " + quote(samples) +
      " --threshold 1e-3 --top-k 64 --max-contacts 4 --repeat 2 --warmup 1 "
      "--csv " + quote(csv) + " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      !std::filesystem::exists(csv) ||
      readFile(stdout_txt).find("avg_reduction_ms") == std::string::npos ||
      readFile(csv).find("avg_reduction_ms") == std::string::npos) {
    std::cerr << "contact reduction benchmark CLI failed\n";
    return 1;
  }
  std::cout << "contact reduction benchmark CLI passed\n";
  return 0;
}

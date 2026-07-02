#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_BENCHMARK_SPARSE_QUERY
#define ADASDF_CL_BENCHMARK_SPARSE_QUERY ""
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
  const std::string benchmark_tool = ADASDF_CL_BENCHMARK_SPARSE_QUERY;
  if (build_tool.empty() || benchmark_tool.empty()) {
    std::cout << "SKIP: sparse benchmark CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "sparse_benchmark_cli_dense.sdfbin";
  const auto stdout_txt = temp / "sparse_benchmark_cli_stdout.txt";
  const auto out_csv = temp / "sparse_benchmark_cli.csv";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const std::string build_cmd = executableCommand(build_tool);
  const std::string benchmark_cmd = executableCommand(benchmark_tool);
  if (std::system((build_cmd + " " + quote(fixture) + " " + quote(model) +
                   " --resolution 24 > " + quote(temp / "sparse_benchmark_build.txt"))
                      .c_str()) != 0) {
    return 1;
  }
  const std::string command =
      benchmark_cmd + " " + quote(model) + " " + quote(samples) +
      " --repeat 2 --warmup 1 --mode phi-only --csv " + quote(out_csv) +
      " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 || !std::filesystem::exists(out_csv) ||
      readFile(stdout_txt).find("avg_ns_per_sample") == std::string::npos ||
      readFile(out_csv).find("avg_ns_per_sample") == std::string::npos) {
    std::cerr << "sparse benchmark CLI failed\n";
    return 1;
  }
  std::cout << "sparse benchmark CLI passed\n";
  return 0;
}

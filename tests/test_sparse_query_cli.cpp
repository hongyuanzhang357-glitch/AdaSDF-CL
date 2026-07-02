#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_SPARSE_QUERY
#define ADASDF_CL_SPARSE_QUERY ""
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
  const std::string build_tool = ADASDF_CL_BUILD_DENSE_SDF;
  const std::string sparse_tool = ADASDF_CL_SPARSE_QUERY;
  if (build_tool.empty() || sparse_tool.empty()) {
    std::cout << "SKIP: sparse query CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "sparse_query_cli_dense.sdfbin";
  const auto stdout_txt = temp / "sparse_query_cli_stdout.txt";
  const auto usage_txt = temp / "sparse_query_cli_usage.txt";
  const auto out_csv = temp / "sparse_query_cli_results.csv";
  const auto report = temp / "sparse_query_cli_report.md";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) / "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";

  const std::string build_cmd = executableCommand(build_tool);
  const std::string sparse_cmd = executableCommand(sparse_tool);
  if (std::system((build_cmd + " " + quote(fixture) + " " + quote(model) +
                   " --resolution 24 > " + quote(temp / "sparse_query_build.txt"))
                      .c_str()) != 0) {
    std::cerr << "failed to build dense model\n";
    return 1;
  }
  if (std::system((sparse_cmd + " > " + quote(usage_txt)).c_str()) != 0 ||
      !contains(readFile(usage_txt), "Usage: adasdf_sparse_query")) {
    std::cerr << "usage failed\n";
    return 1;
  }
  const std::string command =
      sparse_cmd + " " + quote(model) + " " + quote(samples) +
      " --threshold 0 --with-normal --early-exit --out " + quote(out_csv) +
      " --report " + quote(report) + " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 || !std::filesystem::exists(out_csv) ||
      !std::filesystem::exists(report) ||
      !contains(readFile(stdout_txt), "Colliding: true") ||
      !contains(readFile(out_csv), "effective_phi")) {
    std::cerr << "sparse query CLI failed\n";
    return 1;
  }
  std::cout << "sparse query CLI passed\n";
  return 0;
}

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BENCHMARK_HIERARCHICAL_SAMPLING
#define ADASDF_CL_BENCHMARK_HIERARCHICAL_SAMPLING ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
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
  try {
    const std::string benchmark_tool = ADASDF_CL_BENCHMARK_HIERARCHICAL_SAMPLING;
    if (benchmark_tool.empty()) {
      std::cout << "SKIP: hierarchical benchmark was not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const auto stdout_txt = temp / "hierarchical_sampling_benchmark_stdout.txt";
    const auto report = temp / "hierarchical_sampling_benchmark.md";
    const auto json = temp / "hierarchical_sampling_benchmark.json";
    const auto csv = temp / "hierarchical_sampling_benchmark.csv";
    const std::string command =
        executableCommand(benchmark_tool) + " " + quote(fixture) +
        " --max-level 1 --block-resolution 4 --coarse-resolution 2 "
        "--quality-check-samples 2 --comparison-samples 3 "
        "--target-sampling-error 10 --report " + quote(report) +
        " --json " + quote(json) + " --csv " + quote(csv) +
        " > " + quote(stdout_txt);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(report) ||
        !std::filesystem::exists(json) ||
        !std::filesystem::exists(csv)) {
      std::cerr << "hierarchical sampling benchmark CLI failed\n";
      return 1;
    }
    const std::string stdout_text = readFile(stdout_txt);
    if (!contains(stdout_text, "AdaSDF-CL hierarchical sampling benchmark") ||
        !contains(stdout_text, "exact_build_time_ms,hierarchical_build_time_ms,")) {
      std::cerr << "hierarchical sampling benchmark output missing fields\n";
      return 1;
    }

    std::cout << "hierarchical sampling benchmark CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_benchmark_hierarchical_sampling_cli failed: "
              << exc.what() << "\n";
    return 1;
  }
}

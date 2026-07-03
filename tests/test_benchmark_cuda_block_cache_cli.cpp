#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
#endif
#ifndef ADASDF_CL_BENCHMARK_CUDA_BLOCK_CACHE
#define ADASDF_CL_BENCHMARK_CUDA_BLOCK_CACHE ""
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
  if (tool.find('\\') == std::string::npos &&
      tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
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

bool skippedReturnCode(int code) {
  return code == 20 || code == 20 * 256;
}

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
  const std::string benchmark_tool = ADASDF_CL_BENCHMARK_CUDA_BLOCK_CACHE;
  if (build_tool.empty() || benchmark_tool.empty()) {
    std::cout << "SKIP: CUDA block cache benchmark CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "cuda_benchmark_cli_compressed.sdfbin";
  const auto stdout_txt = temp / "cuda_benchmark_cli_stdout.txt";
  const auto csv = temp / "cuda_benchmark_cli.csv";
  const auto report = temp / "cuda_benchmark_cli_report.md";
  const auto json = temp / "cuda_benchmark_cli_report.json";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";

  if (std::system((executableCommand(build_tool) + " " + quote(fixture) +
                   " " + quote(model) +
                   " --max-level 1 --block-resolution 4 --unsigned "
                   "--fixed-rank 2 > " + quote(temp / "cuda_benchmark_build.txt"))
                      .c_str()) != 0) {
    std::cerr << "failed to build compressed model\n";
    return 1;
  }

  const std::string command =
      executableCommand(benchmark_tool) + " " + quote(model) + " " +
      quote(samples) +
      " --repeat 2 --warmup 0 --threshold 1.0 --selection-band 0.1 "
      "--compare-direct --csv " + quote(csv) +
      " --report " + quote(report) +
      " --json " + quote(json) +
      " > " + quote(stdout_txt);
  const int rc = std::system(command.c_str());
  const std::string stdout_text = readFile(stdout_txt);
  const std::string csv_text = readFile(csv);
  if (skippedReturnCode(rc)) {
    if (!contains(stdout_text, "Status: skipped") ||
        !contains(stdout_text, "CUDA available: false") ||
        !std::filesystem::exists(csv) ||
        !contains(csv_text, "cuda_available")) {
      std::cerr << "CUDA block cache benchmark skipped output incomplete\n";
      return 1;
    }
    std::cout << "CUDA block cache benchmark CLI skipped as expected\n";
    return 0;
  }
  if (rc != 0 ||
      !std::filesystem::exists(csv) ||
      !std::filesystem::exists(report) ||
      !std::filesystem::exists(json) ||
      !contains(stdout_text, "Status: ok") ||
      !contains(csv_text, "cuda_kernel_avg_ms") ||
      !contains(csv_text, "cpu_active_avg_ms")) {
    std::cerr << "CUDA block cache benchmark CLI failed with code " << rc
              << "\n";
    return 1;
  }
  std::cout << "CUDA block cache benchmark CLI passed\n";
  return 0;
}

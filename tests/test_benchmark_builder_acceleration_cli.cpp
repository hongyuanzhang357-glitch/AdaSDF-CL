#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BENCHMARK_BUILDER_ACCELERATION
#define ADASDF_CL_BENCHMARK_BUILDER_ACCELERATION ""
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
  const std::string benchmark_tool = ADASDF_CL_BENCHMARK_BUILDER_ACCELERATION;
  if (benchmark_tool.empty()) {
    std::cout << "SKIP: builder acceleration benchmark tool was not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const auto stdout_txt = temp / "builder_acceleration_benchmark_stdout.txt";
  const std::string command =
      executableCommand(benchmark_tool) + " " + quote(fixture) +
      " --builder dense --accel bvh --threads 2 --repeat 1 --warmup 0 "
      "--benchmark-brute-reference --resolution 8 > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0) {
    std::cerr << "builder acceleration benchmark CLI failed\n";
    return 1;
  }
  const std::string text = readFile(stdout_txt);
  if (!contains(text, "AdaSDF-CL builder acceleration benchmark") ||
      !contains(text, "Acceleration: bvh") ||
      !contains(text, "builder,acceleration,threads")) {
    std::cerr << "builder acceleration benchmark output missing fields\n";
    return 1;
  }
  std::cout << "builder acceleration benchmark CLI passed\n";
  return 0;
}

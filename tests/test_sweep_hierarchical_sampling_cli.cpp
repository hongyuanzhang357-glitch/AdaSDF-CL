#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_SWEEP_HIERARCHICAL_SAMPLING
#define ADASDF_CL_SWEEP_HIERARCHICAL_SAMPLING ""
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
    const std::string sweep_tool = ADASDF_CL_SWEEP_HIERARCHICAL_SAMPLING;
    if (sweep_tool.empty()) {
      std::cout << "SKIP: hierarchical sweep was not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "wavy_sphere_ascii.stl";
    const auto stdout_txt = temp / "hierarchical_sampling_sweep_stdout.txt";
    const auto report = temp / "hierarchical_sampling_sweep.md";
    const auto csv = temp / "hierarchical_sampling_sweep.csv";
    const std::string command =
        executableCommand(sweep_tool) + " " + quote(fixture) +
        " --max-level 1 --block-resolution 4 --coarse-resolution 2 "
        "--transition-quality-check-samples 1,2 "
        "--far-field-quality-check corners,sparse "
        "--far-field-safety-factor 2.0 --target-sampling-error 10 "
        "--comparison-samples 3 --csv " + quote(csv) + " --report " +
        quote(report) + " > " + quote(stdout_txt);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(report) ||
        !std::filesystem::exists(csv)) {
      std::cerr << "hierarchical sampling sweep CLI failed\n";
      return 1;
    }
    const std::string stdout_text = readFile(stdout_txt);
    const std::string csv_text = readFile(csv);
    if (!contains(stdout_text, "AdaSDF-CL hierarchical sampling sweep") ||
        !contains(csv_text, "effective_speedup_claim_allowed")) {
      std::cerr << "hierarchical sampling sweep output missing fields\n";
      return 1;
    }

    std::cout << "hierarchical sampling sweep CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_sweep_hierarchical_sampling_cli failed: "
              << exc.what() << "\n";
    return 1;
  }
}

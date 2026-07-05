#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BENCHMARK_CONTACT_BAND_SAMPLING
#define ADASDF_CL_BENCHMARK_CONTACT_BAND_SAMPLING ""
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
    const std::string tool = ADASDF_CL_BENCHMARK_CONTACT_BAND_SAMPLING;
    if (tool.empty()) {
      std::cout << "SKIP: contact-band benchmark was not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    const auto fixture =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
        "closed_cube_ascii.stl";
    const auto stdout_txt = temp / "contact_band_benchmark_stdout.txt";
    const auto report = temp / "contact_band_benchmark.md";
    const auto csv = temp / "contact_band_benchmark.csv";
    const std::string command =
        executableCommand(tool) + " " + quote(fixture) +
        " --max-level 1 --block-resolution 4 --target-error 1e-3 "
        "--contact-band-width 1e-3 --contact-band-layers 1 "
        "--halo-exact-layers 1 --far-field-resolution 2 "
        "--far-field-mode coarse-interpolate --normal-audit "
        "--threads 1 --csv " + quote(csv) + " --report " + quote(report) +
        " > " + quote(stdout_txt);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(report) ||
        !std::filesystem::exists(csv)) {
      std::cerr << "contact-band benchmark CLI failed\n";
      return 1;
    }
    const std::string stdout_text = readFile(stdout_txt);
    if (!contains(stdout_text, "AdaSDF-CL contact-band sampling benchmark") ||
        !contains(stdout_text, "contact_band_quality_passed")) {
      std::cerr << "contact-band benchmark output missing fields\n";
      return 1;
    }
    std::cout << "contact band benchmark CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_band_benchmark_cli failed: "
              << exc.what() << "\n";
    return 1;
  }
}

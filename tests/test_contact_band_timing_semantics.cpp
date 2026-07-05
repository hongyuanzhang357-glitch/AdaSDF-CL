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

int runTimingCase(
    const std::string& tool,
    const std::filesystem::path& fixture,
    const std::filesystem::path& stdout_txt,
    const std::filesystem::path& csv,
    bool exclude_marker) {
  std::string command =
      executableCommand(tool) + " " + quote(fixture) +
      " --max-level 1 --block-resolution 4 --target-error 1e-3 "
      "--contact-band-width 1e-3 --contact-band-layers 1 "
      "--halo-exact-layers 1 --far-field-resolution 2 "
      "--far-field-mode coarse-interpolate --normal-audit "
      "--coverage-audit --coverage-samples-per-axis 2 "
      "--timing-mode end-to-end --include-audit-in-wall-time ";
  command += exclude_marker ? "--exclude-marker-from-speedup "
                            : "--include-marker-in-speedup ";
  command += "--threads 1 --csv " + quote(csv) +
             " > " + quote(stdout_txt);
  return std::system(command.c_str());
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

    const auto default_stdout = temp / "contact_band_timing_default_stdout.txt";
    const auto default_csv = temp / "contact_band_timing_default.csv";
    if (runTimingCase(tool, fixture, default_stdout, default_csv, false) != 0) {
      std::cerr << "default timing benchmark failed\n";
      return 1;
    }
    const std::string default_text = readFile(default_stdout);
    const std::string default_csv_text = readFile(default_csv);
    if (!contains(default_text, "Timing mode: end-to-end") ||
        !contains(default_text, "Include marker in speedup: yes") ||
        !contains(default_text, "Speedup end-to-end:") ||
        !contains(default_text, "Speedup core build:") ||
        !contains(default_csv_text, "speedup_end_to_end") ||
        !contains(default_csv_text, "speedup_core_build") ||
        !contains(default_csv_text, "speedup_excluding_marker") ||
        !contains(default_csv_text, "performance_claim_allowed")) {
      std::cerr << "default timing output missing expected fields\n";
      return 1;
    }

    const auto exclude_stdout = temp / "contact_band_timing_exclude_stdout.txt";
    const auto exclude_csv = temp / "contact_band_timing_exclude.csv";
    if (runTimingCase(tool, fixture, exclude_stdout, exclude_csv, true) != 0) {
      std::cerr << "exclude-marker timing benchmark failed\n";
      return 1;
    }
    const std::string exclude_text = readFile(exclude_stdout);
    if (!contains(exclude_text, "Exclude marker from speedup: yes") ||
        !contains(exclude_text, "Performance claim allowed: no")) {
      std::cerr << "excluding marker should not allow performance claims\n";
      return 1;
    }

    std::cout << "contact-band timing semantics passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_band_timing_semantics failed: "
              << exc.what() << "\n";
    return 1;
  }
}

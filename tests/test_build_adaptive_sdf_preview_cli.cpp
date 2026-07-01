#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_ADAPTIVE_PREVIEW
#define ADASDF_CL_ADAPTIVE_PREVIEW ""
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

}  // namespace

int main() {
  try {
    const std::string tool = ADASDF_CL_ADAPTIVE_PREVIEW;
    if (tool.empty()) {
      std::cout << "SKIP: adaptive SDF preview CLI was not built\n";
      return 0;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);

    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto input = fixture_dir / "closed_cube_ascii.stl";
    const auto output = temp / "adaptive_preview_cli_output.sdfbin";
    const auto plan = temp / "adaptive_preview_cli_plan.md";
    const auto usage_txt = temp / "adaptive_preview_cli_usage.txt";
    const auto dry_run_txt = temp / "adaptive_preview_cli_dry_run.txt";
    const auto non_dry_run_txt = temp / "adaptive_preview_cli_non_dry_run.txt";

    const std::string cmd = executableCommand(tool);
    if (std::system((cmd + " > " + quote(usage_txt)).c_str()) != 0) {
      std::cerr << "adaptive preview usage command failed\n";
      return 1;
    }
    if (!contains(readFile(usage_txt),
                  "Usage: adasdf_build_adaptive_sdf_preview")) {
      std::cerr << "adaptive preview usage output missing\n";
      return 1;
    }

    const std::string dry_run =
        cmd + " " + quote(input) + " " + quote(output) +
        " --target-error 1e-3 --memory-mb 512 --dry-run --plan " +
        quote(plan) + " > " + quote(dry_run_txt);
    if (std::system(dry_run.c_str()) != 0) {
      std::cerr << "adaptive preview dry-run failed\n";
      return 1;
    }
    if (!std::filesystem::exists(plan) ||
        std::filesystem::exists(output) ||
        !contains(readFile(dry_run_txt), "Use adasdf_build_adaptive_sdf") ||
        !contains(readFile(plan),
                  "LowRankCompression implemented in v1.7.0-alpha")) {
      std::cerr << "adaptive preview dry-run output is incomplete\n";
      return 1;
    }

    const std::string non_dry_run =
        cmd + " " + quote(input) + " " + quote(output) +
        " --target-error 1e-3 --memory-mb 512 > " +
        quote(non_dry_run_txt);
    if (std::system(non_dry_run.c_str()) == 0) {
      std::cerr << "adaptive preview non-dry-run unexpectedly succeeded\n";
      return 1;
    }
    if (std::filesystem::exists(output) ||
        !contains(readFile(non_dry_run_txt),
                  "Use adasdf_build_adaptive_sdf")) {
      std::cerr << "adaptive preview non-dry-run failure reason missing\n";
      return 1;
    }

    std::cout << "adaptive SDF preview CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_adaptive_sdf_preview_cli failed: "
              << exc.what() << "\n";
    return 1;
  }
}

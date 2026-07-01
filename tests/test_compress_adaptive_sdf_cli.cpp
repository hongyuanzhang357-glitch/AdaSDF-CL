#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_ADAPTIVE_SDF
#define ADASDF_CL_BUILD_ADAPTIVE_SDF ""
#endif

#ifndef ADASDF_CL_COMPRESS_ADAPTIVE_SDF
#define ADASDF_CL_COMPRESS_ADAPTIVE_SDF ""
#endif

#ifndef ADASDF_CL_INFO
#define ADASDF_CL_INFO ""
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
    const std::string build_tool = ADASDF_CL_BUILD_ADAPTIVE_SDF;
    const std::string compress_tool = ADASDF_CL_COMPRESS_ADAPTIVE_SDF;
    const std::string info_tool = ADASDF_CL_INFO;
    if (build_tool.empty() || compress_tool.empty() || info_tool.empty()) {
      std::cout << "SKIP: compressed SDF CLI tools were not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto adaptive = temp / "compress_cli_adaptive.sdfbin";
    const auto compressed = temp / "compress_cli_compressed.sdfbin";
    const auto report = temp / "compress_cli_report.md";
    const auto json = temp / "compress_cli_report.json";
    const auto quality = temp / "compress_cli_quality.md";
    const auto txt = temp / "compress_cli.txt";
    const auto info = temp / "compress_cli_info.txt";
    const auto usage = temp / "compress_cli_usage.txt";
    const auto bad = temp / "compress_cli_bad.txt";

    const std::string build_cmd = executableCommand(build_tool);
    const std::string compress_cmd = executableCommand(compress_tool);
    const std::string info_cmd = executableCommand(info_tool);
    if (std::system((compress_cmd + " > " + quote(usage)).c_str()) != 0 ||
        !contains(readFile(usage), "Usage: adasdf_compress_adaptive_sdf")) {
      std::cerr << "compress usage output missing\n";
      return 1;
    }

    const std::string build =
        build_cmd + " " + quote(fixture_dir / "closed_cube_ascii.stl") + " " +
        quote(adaptive) + " --max-level 2 --block-resolution 5 > " +
        quote(temp / "compress_cli_build.txt");
    if (std::system(build.c_str()) != 0 || !std::filesystem::exists(adaptive)) {
      std::cerr << "failed to build adaptive input\n";
      return 1;
    }
    const std::string compress =
        compress_cmd + " " + quote(adaptive) + " " + quote(compressed) +
        " --target-error 1e-3 --max-rank 5 --report " + quote(report) +
        " --json " + quote(json) + " --quality-report " + quote(quality) +
        " > " + quote(txt);
    if (std::system(compress.c_str()) != 0 ||
        !std::filesystem::exists(compressed) ||
        !std::filesystem::exists(report) ||
        !std::filesystem::exists(json) ||
        !std::filesystem::exists(quality) ||
        !contains(readFile(txt), "Reload validation: success")) {
      std::cerr << "compress adaptive CLI failed\n";
      return 1;
    }
    if (std::system((info_cmd + " " + quote(compressed) + " > " +
                     quote(info)).c_str()) != 0 ||
        !contains(readFile(info), "Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1")) {
      std::cerr << "info failed on compressed sdfbin\n";
      return 1;
    }
    const std::string bad_cmd =
        compress_cmd + " " + quote(compressed) + " " +
        quote(temp / "bad_output.sdfbin") + " > " + quote(bad) + " 2>&1";
    if (std::system(bad_cmd.c_str()) == 0 ||
        !contains(readFile(bad), "input must be ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")) {
      std::cerr << "non-adaptive input did not fail clearly\n";
      return 1;
    }
    std::cout << "compress adaptive sdf CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compress_adaptive_sdf_cli failed: " << exc.what() << "\n";
    return 1;
  }
}

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_COMPRESSED_SDF
#define ADASDF_CL_BUILD_COMPRESSED_SDF ""
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
    const std::string build_tool = ADASDF_CL_BUILD_COMPRESSED_SDF;
    const std::string info_tool = ADASDF_CL_INFO;
    if (build_tool.empty() || info_tool.empty()) {
      std::cout << "SKIP: build compressed SDF CLI tools were not built\n";
      return 0;
    }
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto output = temp / "build_compressed_cli.sdfbin";
    const auto fixed_output = temp / "build_compressed_cli_fixed.sdfbin";
    const auto report = temp / "build_compressed_cli_report.md";
    const auto compression = temp / "build_compressed_cli_compression.md";
    const auto quality = temp / "build_compressed_cli_quality.md";
    const auto txt = temp / "build_compressed_cli.txt";
    const auto info = temp / "build_compressed_cli_info.txt";

    const std::string build_cmd = executableCommand(build_tool);
    const std::string info_cmd = executableCommand(info_tool);
    const std::string command =
        build_cmd + " " + quote(fixture_dir / "closed_cube_ascii.stl") + " " +
        quote(output) +
        " --target-error 1e-3 --max-level 2 --block-resolution 5 --max-rank 5 "
        "--report " + quote(report) + " --compression-report " +
        quote(compression) + " --quality-report " + quote(quality) +
        " > " + quote(txt);
    if (std::system(command.c_str()) != 0 ||
        !std::filesystem::exists(output) ||
        !std::filesystem::exists(report) ||
        !std::filesystem::exists(compression) ||
        !std::filesystem::exists(quality) ||
        !contains(readFile(txt), "Reload validation: success")) {
      std::cerr << "one-step compressed build failed\n";
      return 1;
    }
    if (std::system((info_cmd + " " + quote(output) + " > " +
                     quote(info)).c_str()) != 0 ||
        !contains(readFile(info), "CompressedBlockSDF block_count:")) {
      std::cerr << "info failed for one-step compressed output\n";
      return 1;
    }
    const std::string fixed =
        build_cmd + " " + quote(fixture_dir / "closed_cube_ascii.stl") + " " +
        quote(fixed_output) +
        " --max-level 2 --block-resolution 5 --fixed-rank 2 > " +
        quote(temp / "build_compressed_cli_fixed.txt");
    if (std::system(fixed.c_str()) != 0 ||
        !std::filesystem::exists(fixed_output)) {
      std::cerr << "fixed-rank compressed build failed\n";
      return 1;
    }
    std::cout << "build compressed sdf CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_compressed_sdf_cli failed: " << exc.what() << "\n";
    return 1;
  }
}

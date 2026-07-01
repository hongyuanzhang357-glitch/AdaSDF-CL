#include <adasdf/adasdf.h>

#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 2;
    options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    if (!model) {
      std::cerr << report.error_message << "\n";
      return 1;
    }

    const std::filesystem::path output = temp / "adaptive_block_roundtrip.sdfbin";
    adasdf::SDFBinWriter::write(output.string(), *model);
    if (!contains(readFile(output), "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")) {
      std::cerr << "adaptive block sdfbin magic missing\n";
      return 1;
    }
    auto reloaded = adasdf::SDFBinReader::read(output);
    auto adaptive = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(reloaded);
    if (!adaptive || adaptive->blockSet().blockCount() != report.block_count ||
        adaptive->metadata().format_name != "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1") {
      std::cerr << "adaptive block roundtrip metadata failed\n";
      return 1;
    }
    const double a = model->sampleDistance({0.5, 0.5, 0.5});
    const double b = reloaded->sampleDistance({0.5, 0.5, 0.5});
    if (std::abs(a - b) > 1.0e-12) {
      std::cerr << "adaptive block roundtrip sample mismatch\n";
      return 1;
    }

    const std::string info_tool = ADASDF_CL_INFO;
    if (!info_tool.empty()) {
      const std::filesystem::path info_txt = temp / "adaptive_block_info.txt";
      if (std::system((executableCommand(info_tool) + " " + quote(output) +
                       " > " + quote(info_txt)).c_str()) != 0 ||
          !contains(readFile(info_txt), "Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1")) {
        std::cerr << "adasdf_info did not recognize adaptive block sdfbin\n";
        return 1;
      }
    }
    std::cout << "adaptive block sdfbin roundtrip passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_block_sdfbin_roundtrip failed: " << exc.what()
              << "\n";
    return 1;
  }
}

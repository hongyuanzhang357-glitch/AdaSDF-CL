#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

int main() {
  try {
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    build_options.max_octree_level = 2;
    build_options.block_resolution = 5;
    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        build_options,
        &build_report);
    auto adaptive = std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
    adasdf::BlockLowRankCompressionOptions options;
    options.max_rank = 5;
    adasdf::BlockLowRankCompressionReport report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            options,
            &report);
    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    const auto output = temp / "compressed_roundtrip.sdfbin";
    adasdf::SDFBinWriter::write(output.string(), compressed_model);
    if (!contains(readFile(output), "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1")) {
      std::cerr << "compressed magic missing\n";
      return 1;
    }
    auto reloaded = adasdf::SDFBinReader::read(output);
    auto compressed_reloaded =
        std::dynamic_pointer_cast<adasdf::CompressedAdaptiveBlockSDFModel>(reloaded);
    if (!compressed_reloaded ||
        compressed_reloaded->compressedBlockSet().blockCount() !=
            compressed_model.compressedBlockSet().blockCount() ||
        compressed_reloaded->metadata().format_name !=
            "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1") {
      std::cerr << "compressed roundtrip metadata failed\n";
      return 1;
    }
    const adasdf::Vector3 p{0.5, 0.5, 0.5};
    if (std::abs(compressed_reloaded->sampleDistance(p) -
                 compressed_model.sampleDistance(p)) > 1.0e-8) {
      std::cerr << "compressed roundtrip query changed\n";
      return 1;
    }
    std::cout << "compressed block sdfbin roundtrip passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compressed_block_sdfbin_roundtrip failed: "
              << exc.what() << "\n";
    return 1;
  }
}

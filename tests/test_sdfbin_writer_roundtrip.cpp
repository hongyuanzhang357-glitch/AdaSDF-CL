#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_CUBE_STL
#define ADASDF_CL_TEST_CUBE_STL ""
#endif

int main() {
  if (!adasdf::ExistingBuilderBridge::isAvailable()) {
    std::cout << "SKIP: existing adaptive builder bridge is not available\n";
    return 0;
  }

  const std::filesystem::path cube = ADASDF_CL_TEST_CUBE_STL;
  if (cube.empty() || !std::filesystem::exists(cube)) {
    std::cout << "SKIP: cube STL fixture is not available\n";
    return 0;
  }

  try {
    adasdf::BuildOptions options;
    options.max_octree_level = 4;
    options.base_block_cells = 8;
    options.min_block_cells = 4;
    options.ghost_cells = 1;
    options.max_rank = 4;
    options.near_min_rank = 2;
    options.verbose = false;

    auto model = adasdf::AdaptiveSDFBuilder::fromSTL(cube.string(), options);
    const std::filesystem::path out =
        std::filesystem::temp_directory_path() / "adasdf_cl_writer_roundtrip.sdfbin";
    adasdf::SDFBinWriter::write(out.string(), *model);
    auto reloaded = adasdf::SDFBinReader::read(out);
    std::error_code ec;
    std::filesystem::remove(out, ec);

    if (!reloaded || !reloaded->isValid()) {
      std::cerr << "Reloaded writer roundtrip model is invalid\n";
      return 1;
    }

    const adasdf::Vector3 center =
        0.5 * (reloaded->boundingBox().min + reloaded->boundingBox().max);
    const double phi = reloaded->sampleDistance(center);
    const adasdf::Vector3 gradient = reloaded->sampleGradient(center);
    if (!std::isfinite(phi) || !gradient.allFinite()) {
      std::cerr << "Reloaded model query returned non-finite values\n";
      return 1;
    }
  } catch (const std::exception& exc) {
    std::cerr << "test_sdfbin_writer_roundtrip failed: " << exc.what() << "\n";
    return 1;
  }

  return 0;
}

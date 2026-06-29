#include <adasdf/adasdf.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  try {
    std::filesystem::path dir = ADASDF_CL_TEST_TEMP_DIR;
    if (dir.empty()) {
      dir = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(dir);
    const std::filesystem::path path = dir / "test_demo_sdfbin_box.sdfbin";

    auto model = adasdf::AnalyticSDFModel::createBox();
    adasdf::SDFBinWriter::write(path.string(), *model);
    if (!adasdf::DemoSDFBin::canRead(path)) {
      std::cerr << "demo sdfbin magic was not detected\n";
      return 1;
    }

    auto reloaded = adasdf::SDFBinReader::read(path);
    if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
      std::cerr << "reloaded demo model is not valid/queryable\n";
      return 1;
    }
    if (reloaded->metadata().format_name != adasdf::DemoSDFBin::magic()) {
      std::cerr << "demo format metadata is missing\n";
      return 1;
    }
    if (!std::isfinite(reloaded->sampleDistance({0.0, 0.0, 0.0})) ||
        !reloaded->sampleGradient({0.75, 0.0, 0.0}).allFinite()) {
      std::cerr << "demo model sampling produced non-finite values\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_demo_sdfbin failed: " << exc.what() << "\n";
    return 1;
  }
}

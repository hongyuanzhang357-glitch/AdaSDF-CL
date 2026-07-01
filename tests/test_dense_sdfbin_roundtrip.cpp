#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  try {
    const std::filesystem::path fixture_dir = ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR;
    const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    std::filesystem::create_directories(temp);
    adasdf::DenseSDFBuildOptions options;
    options.resolution = 20;
    options.padding = 0.05;
    adasdf::DenseSDFBuildReport report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(
        (fixture_dir / "closed_cube_ascii.stl").string(),
        options,
        &report);
    if (!model) {
      std::cerr << report.error_message << "\n";
      return 1;
    }
    const std::filesystem::path output = temp / "dense_roundtrip.sdfbin";
    adasdf::SDFBinWriter::write(output.string(), *model);
    auto reloaded = adasdf::SDFBinReader::read(output);
    auto dense = std::dynamic_pointer_cast<adasdf::DenseSDFModel>(reloaded);
    if (!dense || dense->grid().nx != 20 || dense->metadata().format_name !=
                                   "ADASDF_DENSE_SDFBIN_V1") {
      std::cerr << "dense roundtrip metadata failed\n";
      return 1;
    }
    const double a = model->sampleDistance({0.5, 0.5, 0.5});
    const double b = reloaded->sampleDistance({0.5, 0.5, 0.5});
    if (std::abs(a - b) > 1.0e-12) {
      std::cerr << "roundtrip sample mismatch\n";
      return 1;
    }
    std::cout << "dense sdfbin roundtrip passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_dense_sdfbin_roundtrip failed: " << exc.what() << "\n";
    return 1;
  }
}

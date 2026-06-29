#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <memory>

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
    const std::filesystem::path path = dir / "test_demo_adaptive.sdfbin";

    adasdf::DemoAdaptiveBuildRequest request;
    request.use_surrogate = true;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(request);
    adasdf::SDFBinWriter::write(path.string(), *build.model);

    if (!adasdf::DemoAdaptiveSDFBin::canRead(path)) {
      std::cerr << "demo adaptive sdfbin magic was not detected\n";
      return 1;
    }
    const auto loaded = adasdf::SDFBinReader::read(path);
    const auto adaptive =
        std::dynamic_pointer_cast<adasdf::DemoAdaptiveSDFModel>(loaded);
    if (!adaptive || !adaptive->isValid() || adaptive->blocks().empty() ||
        adaptive->octreeNodes().empty()) {
      std::cerr << "demo adaptive sdfbin did not reload correctly\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_demo_adaptive_sdfbin failed: "
              << exc.what() << "\n";
    return 1;
  }
}

#include "adasdf/io/SDFBinReader.h"

#include <filesystem>
#include <stdexcept>

#include "adasdf/io/AdaptiveBlockSDFBin.h"
#include "adasdf/io/DenseSDFBin.h"
#include "adasdf/io/DemoAdaptiveSDFBin.h"
#include "adasdf/io/DemoSDFBin.h"
#include "adasdf/io/ExistingSDFBridge.h"

namespace adasdf {

std::shared_ptr<SDFModel> SDFBinReader::read(const std::filesystem::path& path) {
  if (path.empty()) {
    throw std::runtime_error("SDFBinReader::read received an empty path.");
  }
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("SDFBinReader::read file does not exist: " + path.string());
  }
  if (AdaptiveBlockSDFBin::canRead(path)) {
    return AdaptiveBlockSDFBin::read(path);
  }
  if (DenseSDFBin::canRead(path)) {
    return DenseSDFBin::read(path);
  }
  if (DemoAdaptiveSDFBin::canRead(path)) {
    return DemoAdaptiveSDFBin::read(path);
  }
  if (DemoSDFBin::canRead(path)) {
    return DemoSDFBin::read(path);
  }
  try {
    return ExistingSDFBridge::loadExistingSDFBin(path);
  } catch (const std::exception& exc) {
    throw std::runtime_error(
        "SDFBinReader::read failed for '" + path.string() + "': " + exc.what());
  }
}

}  // namespace adasdf

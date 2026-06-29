#include "adasdf/io/SDFBinWriter.h"

#include <filesystem>
#include <stdexcept>

#include "adasdf/generation/ExistingBuilderBridge.h"
#include "adasdf/geometry/AnalyticSDFModel.h"
#include "adasdf/io/DemoSDFBin.h"
#include "adasdf/io/SDFBinReader.h"

namespace adasdf {

void SDFBinWriter::write(const std::string& path, const SDFModel& model) {
  if (path.empty()) {
    throw std::runtime_error("SDFBinWriter::write received an empty output path.");
  }
  const std::filesystem::path output(path);
  if (output.has_parent_path()) {
    std::filesystem::create_directories(output.parent_path());
  }

  if (const auto* analytic = dynamic_cast<const AnalyticSDFModel*>(&model)) {
    DemoSDFBin::write(output, *analytic);
    auto reloaded = SDFBinReader::read(output);
    if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
      throw std::runtime_error(
          "SDFBinWriter::write quick validation failed for demo .sdfbin.");
    }
    return;
  }

  if (!ExistingBuilderBridge::writeSDFBin(path, model)) {
    throw std::runtime_error(
        "SDFBinWriter::write could not write this SDFModel. The current native "
        "handle does not expose an existing-format .sdfbin package.");
  }

  auto reloaded = SDFBinReader::read(output);
  if (!reloaded || !reloaded->isValid()) {
    throw std::runtime_error(
        "SDFBinWriter::write quick validation failed: reloaded model is invalid.");
  }
}

}  // namespace adasdf

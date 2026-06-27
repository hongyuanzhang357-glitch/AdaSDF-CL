#include "adasdf/generation/AdaptiveSDFBuilder.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

#include "adasdf/generation/ExistingBuilderBridge.h"

namespace adasdf {
namespace {

std::string lowerExtension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

void requireExistingFile(const std::filesystem::path& path) {
  if (path.empty()) {
    throw std::runtime_error("AdaptiveSDFBuilder received an empty mesh path.");
  }
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("AdaptiveSDFBuilder mesh path does not exist: " +
                             path.string());
  }
}

}  // namespace

std::shared_ptr<SDFModel> AdaptiveSDFBuilder::fromMesh(
    const std::string& mesh_path,
    const BuildOptions& options) {
  const std::filesystem::path path(mesh_path);
  requireExistingFile(path);

  const std::string ext = lowerExtension(path);
  if (ext == ".stl") {
    return fromSTL(mesh_path, options);
  }
  if (ext == ".obj") {
    return fromOBJ(mesh_path, options);
  }

  throw std::runtime_error(
      "AdaptiveSDFBuilder supports .stl in v0.4+. OBJ is a reserved API; "
      "unsupported extension: " +
      ext);
}

std::shared_ptr<SDFModel> AdaptiveSDFBuilder::fromSTL(
    const std::string& stl_path,
    const BuildOptions& options) {
  requireExistingFile(std::filesystem::path(stl_path));
  if (!ExistingBuilderBridge::isAvailable()) {
    throw std::runtime_error(
        "AdaptiveSDFBuilder::fromSTL requires the existing adaptive builder "
        "core. Reconfigure with ADASDF_CL_USE_EXISTING_CORE=ON.");
  }

  auto model = ExistingBuilderBridge::buildFromMesh(stl_path, options);
  if (!model || !model->isValid()) {
    throw std::runtime_error("AdaptiveSDFBuilder::fromSTL produced an invalid model.");
  }
  return model;
}

std::shared_ptr<SDFModel> AdaptiveSDFBuilder::fromOBJ(
    const std::string& obj_path,
    const BuildOptions&) {
  requireExistingFile(std::filesystem::path(obj_path));
  throw std::runtime_error(
      "AdaptiveSDFBuilder::fromOBJ is reserved, but the existing core does not "
      "currently expose a headless OBJ loader bridge.");
}

std::shared_ptr<SDFModel> AdaptiveSDFBuilder::fromMesh(
    const MeshModel&,
    const BuildOptions&) {
  throw std::runtime_error(
      "AdaptiveSDFBuilder::fromMesh(MeshModel) is reserved until MeshModel can "
      "be converted to the existing core mesh type without copying UI or "
      "benchmark code.");
}

}  // namespace adasdf

#include "adasdf/generation/ExistingBuilderBridge.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "adasdf/io/ExistingSDFBridge.h"

#if ADASDF_CL_HAS_EXISTING_CORE
#include "sdf/AdaptiveBlockCompressor.h"
#include "sdf/ModelIO.h"
#include "sdf/OctreeSDF.h"
#include "sdf/SDFModelPackage.h"
#include "sdf/StlReader.h"
#endif

namespace adasdf {
namespace {

#if ADASDF_CL_HAS_EXISTING_CORE

std::string lowerExtension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

sdf::LowRankType lowRankTypeFromOptions(const BuildOptions& options) {
  std::string method = options.compression_method;
  std::transform(method.begin(), method.end(), method.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (method == "svd" || method == "matrix_svd" || method == "matrix-svd") {
    return sdf::LowRankType::MatrixSVD;
  }
  if (method == "tt" || method == "tensor_train" || method == "tensor-train") {
    return sdf::LowRankType::TT;
  }
  return sdf::LowRankType::Tucker;
}

sdf::OctreeSDFOptions toOctreeOptions(const BuildOptions& options) {
  sdf::OctreeSDFOptions out;
  out.maxLevel = options.max_octree_level;
  out.boxMarginRate = options.box_margin_rate;
  out.surfaceBandCells = options.surface_band_cells;
  out.bvhLeafSize = options.bvh_leaf_size;
  out.signRayCount = options.sign_ray_count;
  return out;
}

sdf::AdaptiveCompressionOptions toAdaptiveOptions(
    const BuildOptions& options,
    const sdf::OctreeSDF& octree) {
  sdf::AdaptiveCompressionOptions out;
  out.type = lowRankTypeFromOptions(options);
  out.baseBlockCells = options.base_block_cells;
  out.minBlockCells = options.min_block_cells;
  out.ghostCells = options.ghost_cells;
  out.minRank = options.min_rank;
  out.nearMinRank = options.near_min_rank;
  out.maxRank = options.max_rank;
  out.rankStep = options.rank_step;
  out.rankEnergyPercent = options.rank_energy_percent;
  out.nearBandOverH = options.near_band_over_h;
  if (octree.hFine() > 0.0 && options.near_surface_error > 0.0) {
    // Existing core uses error-over-h. Keep a practical lower bound until
    // absolute-error adaptive policy is exposed directly.
    out.nearErrTolOverH = std::max(options.near_surface_error / octree.hFine(), 0.25);
  }
  out.verbose = options.verbose;
  return out;
}

std::filesystem::path temporarySDFBinPath() {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("adasdf_cl_build_" + std::to_string(stamp) + ".sdfbin");
}

std::shared_ptr<SDFModel> packageToSDFModel(const sdf::SDFModelPackage& package) {
  const std::filesystem::path temp_path = temporarySDFBinPath();
  sdf::ModelIO::saveBinary(temp_path, package);
  auto model = ExistingSDFBridge::loadExistingSDFBin(temp_path);
  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
  return model;
}

#endif

}  // namespace

bool ExistingBuilderBridge::isAvailable() {
#if ADASDF_CL_HAS_EXISTING_CORE
  return true;
#else
  return false;
#endif
}

std::shared_ptr<SDFModel> ExistingBuilderBridge::buildFromMesh(
    const std::string& mesh_path,
    const BuildOptions& options) {
#if ADASDF_CL_HAS_EXISTING_CORE
  if (options.backend == BackendType::CUDA) {
    throw std::runtime_error(
        "ExistingBuilderBridge currently supports CPU construction only.");
  }
  const std::filesystem::path path(mesh_path);
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("Mesh file does not exist: " + mesh_path);
  }
  const std::string ext = lowerExtension(path);
  if (ext != ".stl") {
    throw std::runtime_error(
        "ExistingBuilderBridge v0.4+ currently supports STL input only. "
        "OBJ is reserved for a future mesh loader bridge.");
  }

  sdf::Mesh mesh = sdf::readStl(path);
  if (mesh.empty()) {
    throw std::runtime_error("Mesh loader returned an empty mesh: " + mesh_path);
  }

  const sdf::OctreeSDFOptions octree_options = toOctreeOptions(options);
  sdf::OctreeSDF octree;
  octree.build(mesh, octree_options);

  const sdf::AdaptiveCompressionOptions adaptive_options =
      toAdaptiveOptions(options, octree);
  sdf::SDFModelPackage package;
  package.adaptiveModel =
      sdf::AdaptiveBlockCompressor().build(octree, adaptive_options);
  package.metadata = sdf::makeModelMetadata(
      path.stem().string(),
      path.string(),
      octree,
      octree_options,
      adaptive_options,
      package.adaptiveModel);

  auto model = packageToSDFModel(package);
  if (!model || !model->isValid()) {
    throw std::runtime_error("ExistingBuilderBridge produced an invalid SDFModel.");
  }
  return model;
#else
  (void)mesh_path;
  (void)options;
  throw std::runtime_error(
      "Existing adaptive builder core is not available in this build. "
      "Reconfigure with ADASDF_CL_USE_EXISTING_CORE=ON and dependencies present.");
#endif
}

bool ExistingBuilderBridge::writeSDFBin(
    const std::string& output_path,
    const SDFModel& model) {
  const auto& handle = model.nativeHandle();
  if (!handle || !handle->canWriteSDFBin()) {
    return false;
  }
  handle->writeSDFBin(std::filesystem::path(output_path));
  return true;
}

std::string ExistingBuilderBridge::backendDescription() {
#if ADASDF_CL_HAS_EXISTING_CORE
  return "existing sdf::StlReader + sdf::OctreeSDF + sdf::AdaptiveBlockCompressor + sdf::ModelIO";
#else
  return "unavailable: existing adaptive builder core was not compiled";
#endif
}

}  // namespace adasdf

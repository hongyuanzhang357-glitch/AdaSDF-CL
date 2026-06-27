#include "adasdf/io/ExistingSDFBridge.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if ADASDF_CL_HAS_EXISTING_CORE
#include "sdf/ModelIO.h"
#endif

namespace adasdf {
namespace {

#if ADASDF_CL_HAS_EXISTING_CORE

Vector3 fromSdfVec(const sdf::Vec3& value) {
  return {value.x(), value.y(), value.z()};
}

sdf::Vec3 toSdfVec(const Vector3& value) {
  return sdf::Vec3(value.x, value.y, value.z);
}

SDFMetadata convertMetadata(const sdf::SDFModelPackage& package) {
  const sdf::SDFModelMetadata& source = package.metadata;
  SDFMetadata metadata;
  metadata.model_name = source.modelName;
  metadata.source_path = source.sourceStlPath;
  metadata.query_backend = "existing sdf::ModelIO + sdf::AdaptiveLowRankModel";
  metadata.format_version = sdf::ModelIO::binaryFormatVersion();
  metadata.n_fine_cell = source.nFineCell;
  metadata.n_fine_node = source.nFineNode;
  metadata.h_fine = source.hFine;
  metadata.max_level = source.maxLevel;
  metadata.final_block_count = source.finalBlockCount;
  metadata.active_block_count = source.activeBlockCount;
  metadata.near_surface_block_count = source.nearSurfaceBlockCount;
  metadata.sign_change_block_count = source.signChangeBlockCount;
  metadata.near_surface_error_over_h = source.nearMaxErrOverH;
  metadata.near_surface_error =
      source.hFine > 0.0 ? source.nearMaxErrOverH * source.hFine : source.nearMaxErrOverH;
  metadata.max_reconstruction_error =
      source.hFine > 0.0 ? source.allMaxErrOverH * source.hFine : source.allMaxErrOverH;
  metadata.total_low_rank_memory_mb = source.totalLowRankMemoryMB;
  metadata.total_dense_memory_mb = source.totalDenseMemoryMB;
  metadata.compression_ratio = source.compressionRatio;
  metadata.query_backend_available = true;
  return metadata;
}

std::vector<SDFBlockMetadata> convertBlockMetadata(
    const sdf::SDFModelPackage& package) {
  std::vector<SDFBlockMetadata> blocks;
  const auto& native_blocks = package.adaptiveModel.blocks();
  blocks.reserve(native_blocks.size());
  const double h = package.metadata.hFine;
  const sdf::Vec3 root = package.metadata.rootMin;

  for (const sdf::LowRankBlock& block : native_blocks) {
    const sdf::BlockRange& range = block.range;
    SDFBlockMetadata metadata;
    metadata.block_id = block.blockId;
    metadata.origin = fromSdfVec(root + sdf::Vec3(range.i0 * h, range.j0 * h, range.k0 * h));
    metadata.local_min = fromSdfVec(root + sdf::Vec3(range.ni0 * h, range.nj0 * h, range.nk0 * h));
    metadata.local_max = fromSdfVec(root + sdf::Vec3(range.ni1 * h, range.nj1 * h, range.nk1 * h));
    metadata.resolution = {block.nx, block.ny, block.nz};
    metadata.compression_rank =
        std::max(block.stats.rank1, std::max(block.stats.rank2, block.stats.rank3));
    metadata.near_surface_error = package.metadata.hFine > 0.0
                                      ? block.stats.nearMaxErrOverH * package.metadata.hFine
                                      : block.stats.nearMaxErrOverH;
    metadata.max_reconstruction_error = package.metadata.hFine > 0.0
                                            ? block.stats.allMaxErrOverH * package.metadata.hFine
                                            : block.stats.allMaxErrOverH;
    blocks.push_back(metadata);
  }
  return blocks;
}

class ExistingSDFNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit ExistingSDFNativeHandle(sdf::SDFModelPackage package)
      : package_(std::make_shared<sdf::SDFModelPackage>(std::move(package))) {
    grid_ = sdf::GridInfo{
        package_->metadata.rootMin,
        package_->metadata.rootMax,
        package_->metadata.nFineCell,
        package_->metadata.nFineNode,
        package_->metadata.hFine};
  }

  Scalar sampleDistance(const Vector3& point) const override {
    return package_->adaptiveModel.queryPhi(grid_, toSdfVec(point));
  }

  bool canSampleDistance() const override {
    return package_ && package_->metadata.nFineCell > 0 && package_->metadata.hFine > 0.0;
  }

  Scalar finiteDifferenceStep() const override {
    if (grid_.hFine > 0.0) {
      return std::max(grid_.hFine * 0.5, 1.0e-9);
    }
    return 1.0e-6;
  }

  std::string backendName() const override {
    return "existing sdf::AdaptiveLowRankModel CPU query";
  }

  bool canWriteSDFBin() const override {
    return static_cast<bool>(package_);
  }

  void writeSDFBin(const std::filesystem::path& path) const override {
    if (!package_) {
      throw std::runtime_error("Existing SDF package is not available for writing.");
    }
    sdf::ModelIO::saveBinary(path, *package_);
  }

 private:
  std::shared_ptr<sdf::SDFModelPackage> package_;
  sdf::GridInfo grid_;
};

#endif

}  // namespace

std::shared_ptr<SDFModel> ExistingSDFBridge::loadExistingSDFBin(
    const std::filesystem::path& path) {
#if ADASDF_CL_HAS_EXISTING_CORE
  const auto header = sdf::ModelIO::inspectBinaryHeader(path);
  if (!header.readable) {
    throw std::runtime_error(
        header.errorMessage.empty() ? "unsupported or unreadable existing .sdfbin"
                                    : header.errorMessage);
  }

  sdf::SDFModelPackage package = sdf::ModelIO::loadBinary(path);
  SDFMetadata metadata = convertMetadata(package);

  auto model = std::make_shared<SDFModel>();
  model->setMetadata(metadata);
  model->setBoundingBox(AABB{fromSdfVec(package.metadata.rootMin),
                             fromSdfVec(package.metadata.rootMax),
                             true});
  model->setBlockMetadata(convertBlockMetadata(package));
  model->setNativeHandle(std::make_shared<ExistingSDFNativeHandle>(std::move(package)));
  model->setValid(model->boundingBox().valid && model->queryBackendAvailable());
  return model;
#else
  (void)path;
  throw std::runtime_error(
      "Existing SDF core is not available in this build. Reconfigure with "
      "ADASDF_CL_USE_EXISTING_CORE=ON and ensure the parent core dependencies "
      "can be found.");
#endif
}

bool ExistingSDFBridge::canLoad(const std::filesystem::path& path) {
#if ADASDF_CL_HAS_EXISTING_CORE
  if (!std::filesystem::exists(path)) {
    return false;
  }
  return sdf::ModelIO::inspectBinaryHeader(path).readable;
#else
  (void)path;
  return false;
#endif
}

SDFMetadata ExistingSDFBridge::readMetadata(const std::filesystem::path& path) {
#if ADASDF_CL_HAS_EXISTING_CORE
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("metadata path does not exist: " + path.string());
  }
  const auto header = sdf::ModelIO::inspectBinaryHeader(path);
  if (!header.readable) {
    throw std::runtime_error(
        header.errorMessage.empty() ? "unsupported or unreadable existing .sdfbin"
                                    : header.errorMessage);
  }
  return convertMetadata(sdf::ModelIO::loadBinary(path));
#else
  (void)path;
  throw std::runtime_error("Existing SDF core is not available in this build.");
#endif
}

bool ExistingSDFBridge::existingCoreAvailable() {
#if ADASDF_CL_HAS_EXISTING_CORE
  return true;
#else
  return false;
#endif
}

}  // namespace adasdf

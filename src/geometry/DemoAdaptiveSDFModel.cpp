#include "adasdf/geometry/DemoAdaptiveSDFModel.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

Scalar absScalar(Scalar value) {
  return std::abs(value);
}

Scalar dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Scalar norm(const Vector3& v) {
  return std::sqrt(dot(v, v));
}

Vector3 normalizedOrFallback(const Vector3& v) {
  const Scalar length = norm(v);
  if (!(length > 1.0e-12) || !v.allFinite()) {
    return {1.0, 0.0, 0.0};
  }
  return v / length;
}

Scalar signNonZero(Scalar value) {
  return value < 0.0 ? -1.0 : 1.0;
}

void validateDescription(const DemoAdaptiveSDFDescription& description) {
  if (description.shape_type != "box") {
    throw std::runtime_error(
        "DemoAdaptiveSDFModel currently supports shape_type=box only.");
  }
  if (!description.center.allFinite() || !description.half_extent.allFinite()) {
    throw std::runtime_error("DemoAdaptiveSDFModel parameters must be finite.");
  }
  if (!(description.half_extent.x > 0.0) ||
      !(description.half_extent.y > 0.0) ||
      !(description.half_extent.z > 0.0)) {
    throw std::runtime_error(
        "DemoAdaptiveSDFModel box half extents must be positive.");
  }
  if (!(description.target_near_surface_error > 0.0) ||
      !(description.memory_limit_mb > 0.0) ||
      !(description.block_expand_limit_mb > 0.0)) {
    throw std::runtime_error(
        "DemoAdaptiveSDFModel target and memory limits must be positive.");
  }
}

Scalar boxSignedDistance(
    const Vector3& point,
    const DemoAdaptiveSDFDescription& description) {
  const Vector3 local = point - description.center;
  const Vector3 q{
      absScalar(local.x) - description.half_extent.x,
      absScalar(local.y) - description.half_extent.y,
      absScalar(local.z) - description.half_extent.z};
  const Vector3 outside{
      std::max<Scalar>(q.x, 0.0),
      std::max<Scalar>(q.y, 0.0),
      std::max<Scalar>(q.z, 0.0)};
  return norm(outside) + std::min(std::max({q.x, q.y, q.z}), 0.0);
}

Vector3 boxGradient(
    const Vector3& point,
    const DemoAdaptiveSDFDescription& description) {
  const Vector3 local = point - description.center;
  const Vector3 q{
      absScalar(local.x) - description.half_extent.x,
      absScalar(local.y) - description.half_extent.y,
      absScalar(local.z) - description.half_extent.z};
  const Vector3 outside{
      q.x > 0.0 ? q.x * signNonZero(local.x) : 0.0,
      q.y > 0.0 ? q.y * signNonZero(local.y) : 0.0,
      q.z > 0.0 ? q.z * signNonZero(local.z) : 0.0};
  if (norm(outside) > 1.0e-12) {
    return normalizedOrFallback(outside);
  }

  const Scalar dx = description.half_extent.x - absScalar(local.x);
  const Scalar dy = description.half_extent.y - absScalar(local.y);
  const Scalar dz = description.half_extent.z - absScalar(local.z);
  if (dx <= dy && dx <= dz) {
    return {signNonZero(local.x), 0.0, 0.0};
  }
  if (dy <= dx && dy <= dz) {
    return {0.0, signNonZero(local.y), 0.0};
  }
  return {0.0, 0.0, signNonZero(local.z)};
}

double totalMemoryKb(const std::vector<DemoBlockInfo>& blocks) {
  return std::accumulate(
      blocks.begin(),
      blocks.end(),
      0.0,
      [](double sum, const DemoBlockInfo& block) {
        return sum + std::max(0.0, block.estimated_memory_kb);
      });
}

std::vector<SDFBlockMetadata> toBlockMetadata(
    const std::vector<DemoBlockInfo>& blocks) {
  std::vector<SDFBlockMetadata> metadata;
  metadata.reserve(blocks.size());
  for (const DemoBlockInfo& block : blocks) {
    SDFBlockMetadata item;
    item.block_id = block.block_id;
    item.local_min = block.min_corner;
    item.local_max = block.max_corner;
    item.origin = block.min_corner;
    item.resolution = {block.resolution, block.resolution, block.resolution};
    item.compression_rank = block.rank;
    item.near_surface_error = block.estimated_error;
    item.max_reconstruction_error = block.estimated_error;
    metadata.push_back(item);
  }
  return metadata;
}

class DemoAdaptiveNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit DemoAdaptiveNativeHandle(DemoAdaptiveSDFDescription description)
      : description_(std::move(description)) {}

  Scalar sampleDistance(const Vector3& point) const override {
    return boxSignedDistance(point, description_);
  }

  bool canSampleDistance() const override {
    return true;
  }

  bool canSampleGradient() const override {
    return true;
  }

  Vector3 sampleGradient(const Vector3& point) const override {
    return boxGradient(point, description_);
  }

  Scalar finiteDifferenceStep() const override {
    const Scalar min_extent = std::min(
        {description_.half_extent.x, description_.half_extent.y,
         description_.half_extent.z});
    return std::max(min_extent * 1.0e-5, 1.0e-7);
  }

  std::string backendName() const override {
    return DemoAdaptiveSDFModel::backendName();
  }

 private:
  DemoAdaptiveSDFDescription description_;
};

}  // namespace

DemoAdaptiveSDFModel::DemoAdaptiveSDFModel(
    DemoAdaptiveSDFDescription description,
    std::vector<DemoOctreeNodeInfo> octree_nodes,
    std::vector<DemoBlockInfo> blocks)
    : description_(std::move(description)),
      octree_nodes_(std::move(octree_nodes)),
      blocks_(std::move(blocks)) {
  validateDescription(description_);
  if (octree_nodes_.empty() || blocks_.empty()) {
    throw std::runtime_error(
        "DemoAdaptiveSDFModel requires non-empty octree and block metadata.");
  }

  setBoundingBox(AABB{
      description_.center - description_.half_extent,
      description_.center + description_.half_extent,
      true});
  setBlockMetadata(toBlockMetadata(blocks_));

  const auto max_level = std::max_element(
      octree_nodes_.begin(),
      octree_nodes_.end(),
      [](const DemoOctreeNodeInfo& a, const DemoOctreeNodeInfo& b) {
        return a.level < b.level;
      });
  const auto max_error = std::max_element(
      blocks_.begin(),
      blocks_.end(),
      [](const DemoBlockInfo& a, const DemoBlockInfo& b) {
        return a.estimated_error < b.estimated_error;
      });
  const int max_resolution = std::max_element(
      blocks_.begin(),
      blocks_.end(),
      [](const DemoBlockInfo& a, const DemoBlockInfo& b) {
        return a.resolution < b.resolution;
      })->resolution;

  SDFMetadata metadata;
  metadata.model_name = "demo adaptive analytic box";
  metadata.format_name = formatMagic();
  metadata.format_version = 1;
  metadata.query_backend = backendName();
  metadata.query_backend_available = true;
  metadata.n_fine_cell = max_resolution * max_resolution * max_resolution;
  metadata.n_fine_node = (max_resolution + 1) * (max_resolution + 1) *
      (max_resolution + 1);
  metadata.h_fine =
      (2.0 * std::max({description_.half_extent.x, description_.half_extent.y,
                       description_.half_extent.z})) /
      static_cast<double>(max_resolution);
  metadata.max_level = max_level->level;
  metadata.final_block_count = static_cast<int>(blocks_.size());
  metadata.active_block_count = static_cast<int>(blocks_.size());
  metadata.near_surface_block_count = static_cast<int>(blocks_.size());
  metadata.near_surface_error = description_.target_near_surface_error;
  metadata.near_surface_error_over_h =
      metadata.h_fine > 0.0 ? metadata.near_surface_error / metadata.h_fine : 0.0;
  metadata.max_reconstruction_error = max_error->estimated_error;
  metadata.total_low_rank_memory_mb = totalMemoryKb(blocks_) / 1024.0;
  metadata.compression_ratio = 1.0 + std::max(0.0, 32.0 - blocks_.front().rank) / 8.0;
  setMetadata(metadata);
  setMemoryFootprintBytes(
      sizeof(DemoAdaptiveSDFModel) +
      octree_nodes_.size() * sizeof(DemoOctreeNodeInfo) +
      blocks_.size() * sizeof(DemoBlockInfo) +
      static_cast<std::size_t>(totalMemoryKb(blocks_) * 1024.0));
  setNativeHandle(std::make_shared<DemoAdaptiveNativeHandle>(description_));
  setValid(true);
}

std::shared_ptr<DemoAdaptiveSDFModel> DemoAdaptiveSDFModel::create(
    DemoAdaptiveSDFDescription description,
    std::vector<DemoOctreeNodeInfo> octree_nodes,
    std::vector<DemoBlockInfo> blocks) {
  return std::make_shared<DemoAdaptiveSDFModel>(
      std::move(description),
      std::move(octree_nodes),
      std::move(blocks));
}

void DemoAdaptiveSDFModel::setSourcePath(std::string source_path) {
  SDFMetadata metadata = this->metadata();
  metadata.source_path = std::move(source_path);
  setMetadata(metadata);
}

const char* DemoAdaptiveSDFModel::backendName() {
  return "core-free demo adaptive analytic SDF backend";
}

const char* DemoAdaptiveSDFModel::formatMagic() {
  return "ADASDF_DEMO_ADAPTIVE_SDFBIN_V1";
}

}  // namespace adasdf

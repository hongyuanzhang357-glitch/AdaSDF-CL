#include "adasdf/geometry/AdaptiveBlockSDFModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double finiteOrZero(double value) {
  return std::isfinite(value) ? value : 0.0;
}

double diagonalLength(const AABB& box) {
  const Vector3 d = box.max - box.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

bool containsPoint(const AABB& box, const Vector3& p) {
  const double eps = 1.0e-12;
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

Vector3 clampToAABB(const AABB& box, const Vector3& p) {
  return {
      std::clamp(p.x, box.min.x, box.max.x),
      std::clamp(p.y, box.min.y, box.max.y),
      std::clamp(p.z, box.min.z, box.max.z)};
}

double distance(const Vector3& a, const Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double sampleBlock(const AdaptiveSDFBlock& block, const Vector3& point) {
  const double x = axisCoord(point.x, block.origin.x, block.spacing.x, block.nx);
  const double y = axisCoord(point.y, block.origin.y, block.spacing.y, block.ny);
  const double z = axisCoord(point.z, block.origin.z, block.spacing.z, block.nz);

  const int i0 = static_cast<int>(std::floor(x));
  const int j0 = static_cast<int>(std::floor(y));
  const int k0 = static_cast<int>(std::floor(z));
  const int i1 = std::min(i0 + 1, block.nx - 1);
  const int j1 = std::min(j0 + 1, block.ny - 1);
  const int k1 = std::min(k0 + 1, block.nz - 1);
  const double tx = x - static_cast<double>(i0);
  const double ty = y - static_cast<double>(j0);
  const double tz = z - static_cast<double>(k0);

  const auto value = [&](int i, int j, int k) {
    return finiteOrZero(
        block.phi[valueIndex(i, j, k, block.nx, block.ny)]);
  };

  const double c00 = lerp(value(i0, j0, k0), value(i1, j0, k0), tx);
  const double c10 = lerp(value(i0, j1, k0), value(i1, j1, k0), tx);
  const double c01 = lerp(value(i0, j0, k1), value(i1, j0, k1), tx);
  const double c11 = lerp(value(i0, j1, k1), value(i1, j1, k1), tx);
  const double c0 = lerp(c00, c10, ty);
  const double c1 = lerp(c01, c11, ty);
  return finiteOrZero(lerp(c0, c1, tz));
}

int findContainingBlockImpl(
    const AdaptiveSDFBlockSet& block_set,
    const Vector3& p) {
  int best = -1;
  int best_level = std::numeric_limits<int>::min();
  double best_diag = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < block_set.blocks.size(); ++i) {
    const AdaptiveSDFBlock& block = block_set.blocks[i];
    if (!containsPoint(block.bounds, p)) {
      continue;
    }
    const double diag = diagonalLength(block.bounds);
    if (block.level > best_level ||
        (block.level == best_level && diag < best_diag)) {
      best = static_cast<int>(i);
      best_level = block.level;
      best_diag = diag;
    }
  }
  return best;
}

double sampleBlockSet(
    const AdaptiveSDFBlockSet& block_set,
    const Vector3& point) {
  const int block_index = findContainingBlockImpl(block_set, point);
  if (block_index >= 0) {
    return sampleBlock(
        block_set.blocks[static_cast<std::size_t>(block_index)],
        point);
  }

  if (block_set.global_bounds.valid && containsPoint(block_set.global_bounds, point)) {
    return 0.0;
  }
  if (!block_set.global_bounds.valid) {
    return 0.0;
  }
  const Vector3 clamped = clampToAABB(block_set.global_bounds, point);
  const int clamped_index = findContainingBlockImpl(block_set, clamped);
  const double boundary_phi =
      clamped_index >= 0
          ? std::abs(sampleBlock(
                block_set.blocks[static_cast<std::size_t>(clamped_index)],
                clamped))
          : 0.0;
  return finiteOrZero(boundary_phi + distance(point, clamped));
}

double oneAxisDerivative(
    const AdaptiveSDFBlockSet& block_set,
    const Vector3& point,
    int axis) {
  const int block_index = findContainingBlockImpl(block_set, point);
  double h = 1.0e-5;
  if (block_index >= 0) {
    const AdaptiveSDFBlock& block =
        block_set.blocks[static_cast<std::size_t>(block_index)];
    h = axis == 0 ? block.spacing.x : (axis == 1 ? block.spacing.y : block.spacing.z);
    h = std::max(h * 0.5, 1.0e-7);
  }
  Vector3 minus = point;
  Vector3 plus = point;
  double* minus_axis = axis == 0 ? &minus.x : (axis == 1 ? &minus.y : &minus.z);
  double* plus_axis = axis == 0 ? &plus.x : (axis == 1 ? &plus.y : &plus.z);
  *minus_axis -= h;
  *plus_axis += h;
  const double denom = *plus_axis - *minus_axis;
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    return 0.0;
  }
  return finiteOrZero(
      (sampleBlockSet(block_set, plus) - sampleBlockSet(block_set, minus)) /
      denom);
}

class AdaptiveBlockNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit AdaptiveBlockNativeHandle(std::shared_ptr<AdaptiveSDFBlockSet> blocks)
      : blocks_(std::move(blocks)) {}

  Scalar sampleDistance(const Vector3& point) const override {
    return sampleBlockSet(*blocks_, point);
  }

  bool canSampleDistance() const override {
    return blocks_ && AdaptiveBlockSDFModel::isValidBlockSet(*blocks_);
  }

  bool canSampleGradient() const override {
    return canSampleDistance();
  }

  Vector3 sampleGradient(const Vector3& point) const override {
    return {
        oneAxisDerivative(*blocks_, point, 0),
        oneAxisDerivative(*blocks_, point, 1),
        oneAxisDerivative(*blocks_, point, 2)};
  }

  Scalar finiteDifferenceStep() const override {
    if (!blocks_ || blocks_->blocks.empty()) {
      return 1.0e-6;
    }
    double h = std::numeric_limits<double>::infinity();
    for (const AdaptiveSDFBlock& block : blocks_->blocks) {
      h = std::min({h, block.spacing.x, block.spacing.y, block.spacing.z});
    }
    return std::max(h * 0.5, 1.0e-7);
  }

  std::string backendName() const override {
    return AdaptiveBlockSDFModel::backendName();
  }

 private:
  std::shared_ptr<AdaptiveSDFBlockSet> blocks_;
};

std::vector<SDFBlockMetadata> makeBlockMetadata(
    const AdaptiveSDFBlockSet& blocks) {
  std::vector<SDFBlockMetadata> metadata;
  metadata.reserve(blocks.blocks.size());
  for (const AdaptiveSDFBlock& block : blocks.blocks) {
    SDFBlockMetadata item;
    item.block_id = block.block_id;
    item.origin = block.origin;
    item.local_min = block.bounds.min;
    item.local_max = block.bounds.max;
    item.resolution = {block.nx, block.ny, block.nz};
    item.near_surface_error = block.near_surface ? 1.0 : 0.0;
    metadata.push_back(item);
  }
  return metadata;
}

}  // namespace

AdaptiveBlockSDFModel::AdaptiveBlockSDFModel()
    : block_set_(std::make_shared<AdaptiveSDFBlockSet>()) {
  setValid(false);
}

AdaptiveBlockSDFModel::AdaptiveBlockSDFModel(AdaptiveSDFBlockSet blocks)
    : block_set_(std::make_shared<AdaptiveSDFBlockSet>(std::move(blocks))) {
  if (!isValidBlockSet(*block_set_)) {
    setValid(false);
    return;
  }
  setBoundingBox(block_set_->global_bounds);
  setBlockMetadata(makeBlockMetadata(*block_set_));

  SDFMetadata metadata;
  metadata.model_name = "adaptive block dense SDF";
  metadata.format_name = "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1";
  metadata.format_version = 1;
  metadata.query_backend = backendName();
  metadata.query_backend_available = true;
  metadata.final_block_count = static_cast<int>(block_set_->blocks.size());
  metadata.active_block_count = metadata.final_block_count;
  metadata.near_surface_block_count = static_cast<int>(std::count_if(
      block_set_->blocks.begin(),
      block_set_->blocks.end(),
      [](const AdaptiveSDFBlock& block) { return block.near_surface; }));
  metadata.max_level = 0;
  int min_resolution = std::numeric_limits<int>::max();
  for (const AdaptiveSDFBlock& block : block_set_->blocks) {
    metadata.max_level = std::max(metadata.max_level, block.level);
    min_resolution = std::min(min_resolution, block.nx);
  }
  metadata.n_fine_node =
      min_resolution == std::numeric_limits<int>::max() ? 0 : min_resolution;
  metadata.n_fine_cell = std::max(0, metadata.n_fine_node - 1);
  metadata.total_dense_memory_mb =
      static_cast<double>(block_set_->memoryFootprintBytes()) /
      (1024.0 * 1024.0);
  setMetadata(metadata);
  setMemoryFootprintBytes(
      sizeof(AdaptiveBlockSDFModel) + block_set_->memoryFootprintBytes());
  setNativeHandle(std::make_shared<AdaptiveBlockNativeHandle>(block_set_));
  setValid(true);
}

const AdaptiveSDFBlockSet& AdaptiveBlockSDFModel::blockSet() const {
  return *block_set_;
}

AdaptiveSDFBlockSet& AdaptiveBlockSDFModel::blockSet() {
  return *block_set_;
}

int AdaptiveBlockSDFModel::findContainingBlock(const Vector3& p) const {
  return findContainingBlockImpl(*block_set_, p);
}

bool AdaptiveBlockSDFModel::isValidBlockSet(
    const AdaptiveSDFBlockSet& blocks) {
  if (!blocks.global_bounds.valid || !blocks.global_bounds.min.allFinite() ||
      !blocks.global_bounds.max.allFinite()) {
    return false;
  }
  if (blocks.blocks.empty()) {
    return false;
  }
  for (const AdaptiveSDFBlock& block : blocks.blocks) {
    if (block.block_id < 0 || block.nx < 2 || block.ny < 2 || block.nz < 2) {
      return false;
    }
    if (!block.bounds.valid || !block.origin.allFinite() ||
        !block.spacing.allFinite()) {
      return false;
    }
    if (!(block.spacing.x > 0.0) || !(block.spacing.y > 0.0) ||
        !(block.spacing.z > 0.0)) {
      return false;
    }
    const std::size_t expected =
        static_cast<std::size_t>(block.nx) *
        static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz);
    if (block.phi.size() != expected) {
      return false;
    }
    if (!std::all_of(block.phi.begin(), block.phi.end(), [](double phi) {
          return std::isfinite(phi);
        })) {
      return false;
    }
  }
  return true;
}

const char* AdaptiveBlockSDFModel::backendName() {
  return "core-free adaptive block dense SDF backend";
}

}  // namespace adasdf

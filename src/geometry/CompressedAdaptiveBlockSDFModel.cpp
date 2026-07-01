#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace adasdf {
namespace {

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

double sampleBlock(const CompressedSDFBlock& block, const Vector3& point) {
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
    return finiteOrZero(compressedBlockGridValue(block, i, j, k));
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
    const CompressedAdaptiveBlockSDF& compressed,
    const Vector3& p) {
  int best = -1;
  int best_level = std::numeric_limits<int>::min();
  double best_diag = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < compressed.blocks.size(); ++i) {
    const CompressedSDFBlock& block = compressed.blocks[i];
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
    const CompressedAdaptiveBlockSDF& compressed,
    const Vector3& point) {
  const int block_index = findContainingBlockImpl(compressed, point);
  if (block_index >= 0) {
    return sampleBlock(
        compressed.blocks[static_cast<std::size_t>(block_index)],
        point);
  }

  if (compressed.global_bounds.valid && containsPoint(compressed.global_bounds, point)) {
    return 0.0;
  }
  if (!compressed.global_bounds.valid) {
    return 0.0;
  }
  const Vector3 clamped = clampToAABB(compressed.global_bounds, point);
  const int clamped_index = findContainingBlockImpl(compressed, clamped);
  const double boundary_phi =
      clamped_index >= 0
          ? std::abs(sampleBlock(
                compressed.blocks[static_cast<std::size_t>(clamped_index)],
                clamped))
          : 0.0;
  return finiteOrZero(boundary_phi + distance(point, clamped));
}

double oneAxisDerivative(
    const CompressedAdaptiveBlockSDF& compressed,
    const Vector3& point,
    int axis) {
  const int block_index = findContainingBlockImpl(compressed, point);
  double h = 1.0e-5;
  if (block_index >= 0) {
    const CompressedSDFBlock& block =
        compressed.blocks[static_cast<std::size_t>(block_index)];
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
      (sampleBlockSet(compressed, plus) - sampleBlockSet(compressed, minus)) /
      denom);
}

class CompressedAdaptiveBlockNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit CompressedAdaptiveBlockNativeHandle(
      std::shared_ptr<CompressedAdaptiveBlockSDF> compressed)
      : compressed_(std::move(compressed)) {}

  Scalar sampleDistance(const Vector3& point) const override {
    return sampleBlockSet(*compressed_, point);
  }

  bool canSampleDistance() const override {
    return compressed_ &&
           CompressedAdaptiveBlockSDFModel::isValidCompressedBlockSet(*compressed_);
  }

  bool canSampleGradient() const override {
    return canSampleDistance();
  }

  Vector3 sampleGradient(const Vector3& point) const override {
    return {
        oneAxisDerivative(*compressed_, point, 0),
        oneAxisDerivative(*compressed_, point, 1),
        oneAxisDerivative(*compressed_, point, 2)};
  }

  Scalar finiteDifferenceStep() const override {
    if (!compressed_ || compressed_->blocks.empty()) {
      return 1.0e-6;
    }
    double h = std::numeric_limits<double>::infinity();
    for (const CompressedSDFBlock& block : compressed_->blocks) {
      h = std::min({h, block.spacing.x, block.spacing.y, block.spacing.z});
    }
    return std::max(h * 0.5, 1.0e-7);
  }

  std::string backendName() const override {
    return CompressedAdaptiveBlockSDFModel::backendName();
  }

 private:
  std::shared_ptr<CompressedAdaptiveBlockSDF> compressed_;
};

std::vector<SDFBlockMetadata> makeBlockMetadata(
    const CompressedAdaptiveBlockSDF& compressed) {
  std::vector<SDFBlockMetadata> metadata;
  metadata.reserve(compressed.blocks.size());
  for (const CompressedSDFBlock& block : compressed.blocks) {
    SDFBlockMetadata item;
    item.block_id = block.block_id;
    item.origin = block.origin;
    item.local_min = block.bounds.min;
    item.local_max = block.bounds.max;
    item.resolution = {block.nx, block.ny, block.nz};
    item.compression_rank =
        block.method == BlockCompressionMethod::MatrixSVD ? block.svd.rank : 0;
    item.near_surface_error = block.near_surface ? 1.0 : 0.0;
    item.max_reconstruction_error = block.max_abs_error;
    metadata.push_back(item);
  }
  return metadata;
}

}  // namespace

CompressedAdaptiveBlockSDFModel::CompressedAdaptiveBlockSDFModel()
    : compressed_(std::make_shared<CompressedAdaptiveBlockSDF>()) {
  setValid(false);
}

CompressedAdaptiveBlockSDFModel::CompressedAdaptiveBlockSDFModel(
    CompressedAdaptiveBlockSDF compressed)
    : compressed_(
          std::make_shared<CompressedAdaptiveBlockSDF>(std::move(compressed))) {
  if (!isValidCompressedBlockSet(*compressed_)) {
    setValid(false);
    return;
  }
  setBoundingBox(compressed_->global_bounds);
  setBlockMetadata(makeBlockMetadata(*compressed_));

  SDFMetadata metadata;
  metadata.model_name = "compressed adaptive block SDF";
  metadata.format_name = "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1";
  metadata.format_version = 1;
  metadata.query_backend = backendName();
  metadata.query_backend_available = true;
  metadata.final_block_count = static_cast<int>(compressed_->blocks.size());
  metadata.active_block_count = metadata.final_block_count;
  metadata.near_surface_block_count = static_cast<int>(std::count_if(
      compressed_->blocks.begin(),
      compressed_->blocks.end(),
      [](const CompressedSDFBlock& block) { return block.near_surface; }));
  metadata.max_level = 0;
  int min_resolution = std::numeric_limits<int>::max();
  double max_error = 0.0;
  for (const CompressedSDFBlock& block : compressed_->blocks) {
    metadata.max_level = std::max(metadata.max_level, block.level);
    min_resolution = std::min(min_resolution, block.nx);
    max_error = std::max(max_error, block.max_abs_error);
  }
  metadata.n_fine_node =
      min_resolution == std::numeric_limits<int>::max() ? 0 : min_resolution;
  metadata.n_fine_cell = std::max(0, metadata.n_fine_node - 1);
  metadata.max_reconstruction_error = max_error;
  metadata.total_dense_memory_mb =
      static_cast<double>(compressed_->originalMemoryBytes()) / (1024.0 * 1024.0);
  metadata.total_low_rank_memory_mb =
      static_cast<double>(compressed_->compressedMemoryBytes()) / (1024.0 * 1024.0);
  metadata.compression_ratio = compressed_->compressionRatio();
  setMetadata(metadata);
  setMemoryFootprintBytes(
      sizeof(CompressedAdaptiveBlockSDFModel) +
      compressed_->compressedMemoryBytes());
  setNativeHandle(
      std::make_shared<CompressedAdaptiveBlockNativeHandle>(compressed_));
  setValid(true);
}

const CompressedAdaptiveBlockSDF&
CompressedAdaptiveBlockSDFModel::compressedBlockSet() const {
  return *compressed_;
}

CompressedAdaptiveBlockSDF&
CompressedAdaptiveBlockSDFModel::compressedBlockSet() {
  return *compressed_;
}

int CompressedAdaptiveBlockSDFModel::findContainingBlock(
    const Vector3& p) const {
  return findContainingBlockImpl(*compressed_, p);
}

bool CompressedAdaptiveBlockSDFModel::isValidCompressedBlockSet(
    const CompressedAdaptiveBlockSDF& compressed) {
  if (!compressed.global_bounds.valid ||
      !compressed.global_bounds.min.allFinite() ||
      !compressed.global_bounds.max.allFinite()) {
    return false;
  }
  if (compressed.blocks.empty()) {
    return false;
  }
  for (const CompressedSDFBlock& block : compressed.blocks) {
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
    if (block.method == BlockCompressionMethod::DenseFallback) {
      if (block.dense_phi.size() != expected) {
        return false;
      }
      if (!std::all_of(block.dense_phi.begin(), block.dense_phi.end(), [](double phi) {
            return std::isfinite(phi);
          })) {
        return false;
      }
    } else if (block.method == BlockCompressionMethod::MatrixSVD) {
      if (block.svd.rows != block.nx || block.svd.cols != block.ny * block.nz ||
          block.svd.rank <= 0) {
        return false;
      }
      if (block.svd.U.size() !=
              static_cast<std::size_t>(block.svd.rows) *
                  static_cast<std::size_t>(block.svd.rank) ||
          block.svd.S.size() != static_cast<std::size_t>(block.svd.rank) ||
          block.svd.Vt.size() !=
              static_cast<std::size_t>(block.svd.rank) *
                  static_cast<std::size_t>(block.svd.cols)) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

const char* CompressedAdaptiveBlockSDFModel::backendName() {
  return "core-free compressed adaptive block SDF backend";
}

}  // namespace adasdf

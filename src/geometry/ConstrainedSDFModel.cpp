#include "adasdf/geometry/ConstrainedSDFModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

bool contains(const AABB& box, const Vector3& point) {
  const double eps = 1.0e-12;
  return box.valid && point.x >= box.min.x - eps &&
      point.x <= box.max.x + eps && point.y >= box.min.y - eps &&
      point.y <= box.max.y + eps && point.z >= box.min.z - eps &&
      point.z <= box.max.z + eps;
}

Vector3 clampPoint(const AABB& box, const Vector3& point) {
  return {
      std::clamp(point.x, box.min.x, box.max.x),
      std::clamp(point.y, box.min.y, box.max.y),
      std::clamp(point.z, box.min.z, box.max.z)};
}

double pointDistance(const Vector3& lhs, const Vector3& rhs) {
  const Vector3 delta = lhs - rhs;
  return std::sqrt(
      delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
}

double axisCoordinate(double value, double lo, double hi, int n) {
  if (n <= 1 || !(hi > lo)) {
    return 0.0;
  }
  return std::clamp(
      (value - lo) / (hi - lo) * static_cast<double>(n - 1),
      0.0,
      static_cast<double>(n - 1));
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

int containingBlockLinear(
    const std::vector<ConstrainedSDFBinBlockRecord>& blocks,
    const Vector3& point) {
  int selected = -1;
  int selected_level = std::numeric_limits<int>::min();
  for (std::size_t i = 0; i < blocks.size(); ++i) {
    if (!contains(blocks[i].bounds, point)) {
      continue;
    }
    if (blocks[i].level > selected_level ||
        (blocks[i].level == selected_level &&
         (selected < 0 || blocks[i].block_id <
              blocks[static_cast<std::size_t>(selected)].block_id))) {
      selected = static_cast<int>(i);
      selected_level = blocks[i].level;
    }
  }
  return selected;
}

class ConstrainedNativeHandle final : public SDFModel::NativeHandle {
 public:
  explicit ConstrainedNativeHandle(
      std::shared_ptr<ConstrainedSDFBinReader> reader,
      ConstrainedSDFQueryStorage storage)
      : reader_(std::move(reader)), storage_(storage) {
    if (reader_ == nullptr) {
      return;
    }
    node_to_block_indices_.resize(reader_->octreeNodes().size());
    cache_.reserve(reader_->blocks().size());
    for (std::size_t block_index = 0; block_index < reader_->blocks().size();
         ++block_index) {
      cache_.push_back(std::make_unique<CachedTensor>());
      const int node_id = reader_->blocks()[block_index].source_octree_node_id;
      if (node_id >= 0 &&
          static_cast<std::size_t>(node_id) < node_to_block_indices_.size()) {
        node_to_block_indices_[static_cast<std::size_t>(node_id)].push_back(
            static_cast<int>(block_index));
      } else {
        octree_lookup_valid_ = false;
      }
      const ConstrainedSDFBinBlockRecord& block =
          reader_->blocks()[block_index];
      minimum_spacing_ = std::min({
          minimum_spacing_,
          (block.bounds.max.x - block.bounds.min.x) /
              static_cast<double>(std::max(1, block.nx - 1)),
          (block.bounds.max.y - block.bounds.min.y) /
              static_cast<double>(std::max(1, block.ny - 1)),
          (block.bounds.max.z - block.bounds.min.z) /
              static_cast<double>(std::max(1, block.nz - 1))});
    }
    octree_lookup_valid_ = octree_lookup_valid_ &&
        !reader_->octreeNodes().empty() &&
        reader_->octreeNodes().front().node_id == 0;
  }

  Scalar sampleDistance(const Vector3& point) const override {
    if (!canSampleDistance()) {
      throw std::runtime_error("invalid constrained SDF runtime reader");
    }
    const bool outside = !contains(reader_->bounds(), point);
    const Vector3 query = outside ? clampPoint(reader_->bounds(), point) : point;
    const int block_index = containingBlock(query);
    if (block_index < 0) {
      throw std::runtime_error("constrained SDF block lookup failed");
    }
    const double phi = sampleBlock(static_cast<std::size_t>(block_index), query);
    return outside ? std::abs(phi) + pointDistance(point, query) : phi;
  }

  bool canSampleDistance() const override {
    return reader_ != nullptr && reader_->valid() && !reader_->blocks().empty();
  }

  Scalar finiteDifferenceStep() const override {
    if (!canSampleDistance()) {
      return 1.0e-6;
    }
    return std::isfinite(minimum_spacing_)
        ? std::max(minimum_spacing_ * 0.5, 1.0e-8)
        : 1.0e-6;
  }

  std::string backendName() const override {
    return ConstrainedSDFModel::backendName();
  }

 private:
  struct CachedTensor {
    std::once_flag once;
    CompressedTensor3D tensor;
    std::vector<double> decoded;
    std::string error;
    bool loaded = false;
  };

  void visitContainingBlocks(
      int node_id,
      const Vector3& point,
      int* selected,
      int* selected_level,
      int* selected_block_id) const {
    if (node_id < 0 ||
        static_cast<std::size_t>(node_id) >= reader_->octreeNodes().size()) {
      return;
    }
    const ConstrainedAdaptiveOctreeNode& node =
        reader_->octreeNodes()[static_cast<std::size_t>(node_id)];
    if (!contains(node.bounds, point)) {
      return;
    }
    for (const int block_index :
         node_to_block_indices_[static_cast<std::size_t>(node_id)]) {
      const ConstrainedSDFBinBlockRecord& block =
          reader_->blocks()[static_cast<std::size_t>(block_index)];
      if (contains(block.bounds, point) &&
          (block.level > *selected_level ||
           (block.level == *selected_level &&
            block.block_id < *selected_block_id))) {
        *selected = block_index;
        *selected_level = block.level;
        *selected_block_id = block.block_id;
      }
    }
    const Vector3 center = 0.5 * (node.bounds.min + node.bounds.max);
    const auto axisBits = [](double value, double split, int high_bit) {
      std::array<int, 2> bits{{0, 0}};
      int count = 1;
      if (std::abs(value - split) <= 1.0e-12) {
        bits[1] = high_bit;
        count = 2;
      } else if (value > split) {
        bits[0] = high_bit;
      }
      return std::pair<std::array<int, 2>, int>{bits, count};
    };
    const auto x_bits = axisBits(point.x, center.x, 1);
    const auto y_bits = axisBits(point.y, center.y, 2);
    const auto z_bits = axisBits(point.z, center.z, 4);
    for (int z = 0; z < z_bits.second; ++z) {
      for (int y = 0; y < y_bits.second; ++y) {
        for (int x = 0; x < x_bits.second; ++x) {
          const int child = x_bits.first[static_cast<std::size_t>(x)] |
              y_bits.first[static_cast<std::size_t>(y)] |
              z_bits.first[static_cast<std::size_t>(z)];
          const int child_id = node.child_ids[static_cast<std::size_t>(child)];
          if (child_id >= 0) {
            visitContainingBlocks(
                child_id,
                point,
                selected,
                selected_level,
                selected_block_id);
          }
        }
      }
    }
  }

  int containingBlock(const Vector3& point) const {
    if (octree_lookup_valid_) {
      int selected = -1;
      int selected_level = std::numeric_limits<int>::min();
      int selected_block_id = std::numeric_limits<int>::max();
      visitContainingBlocks(
          0, point, &selected, &selected_level, &selected_block_id);
      if (selected >= 0) {
        return selected;
      }
    }
    return containingBlockLinear(reader_->blocks(), point);
  }

  double sampleBlock(std::size_t block_index, const Vector3& point) const {
    CachedTensor& cached = *cache_[block_index];
    std::call_once(cached.once, [&]() {
      cached.loaded =
          reader_->loadBlock(block_index, &cached.tensor, &cached.error);
      if (cached.loaded &&
          storage_ == ConstrainedSDFQueryStorage::DecodedCache) {
        cached.decoded = AdaptiveTensorCompressor::decode(cached.tensor);
        cached.loaded = !cached.decoded.empty();
        if (!cached.loaded) {
          cached.error = "failed to decode constrained SDF tensor";
        }
      }
    });
    if (!cached.loaded) {
      throw std::runtime_error(cached.error);
    }
    const CompressedTensor3D& tensor = cached.tensor;
    const ConstrainedSDFBinBlockRecord& block = reader_->blocks()[block_index];
    const double x = axisCoordinate(
        point.x, block.bounds.min.x, block.bounds.max.x, tensor.nx);
    const double y = axisCoordinate(
        point.y, block.bounds.min.y, block.bounds.max.y, tensor.ny);
    const double z = axisCoordinate(
        point.z, block.bounds.min.z, block.bounds.max.z, tensor.nz);
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int z0 = static_cast<int>(std::floor(z));
    const int x1 = std::min(x0 + 1, tensor.nx - 1);
    const int y1 = std::min(y0 + 1, tensor.ny - 1);
    const int z1 = std::min(z0 + 1, tensor.nz - 1);
    const double tx = x - static_cast<double>(x0);
    const double ty = y - static_cast<double>(y0);
    const double tz = z - static_cast<double>(z0);
    if (cached.decoded.empty()) {
      return AdaptiveTensorCompressor::sampleTrilinear(tensor, x, y, z);
    }
    const auto value = [&](int ix, int iy, int iz) {
      const std::size_t index = static_cast<std::size_t>(ix) +
          static_cast<std::size_t>(tensor.nx) *
              (static_cast<std::size_t>(iy) +
               static_cast<std::size_t>(tensor.ny) *
                   static_cast<std::size_t>(iz));
      return cached.decoded[index];
    };
    const double c00 = lerp(value(x0, y0, z0), value(x1, y0, z0), tx);
    const double c10 = lerp(value(x0, y1, z0), value(x1, y1, z0), tx);
    const double c01 = lerp(value(x0, y0, z1), value(x1, y0, z1), tx);
    const double c11 = lerp(value(x0, y1, z1), value(x1, y1, z1), tx);
    return lerp(lerp(c00, c10, ty), lerp(c01, c11, ty), tz);
  }

  std::shared_ptr<ConstrainedSDFBinReader> reader_;
  ConstrainedSDFQueryStorage storage_ =
      ConstrainedSDFQueryStorage::CompressedDirect;
  std::vector<std::vector<int>> node_to_block_indices_;
  bool octree_lookup_valid_ = true;
  double minimum_spacing_ = std::numeric_limits<double>::infinity();
  mutable std::vector<std::unique_ptr<CachedTensor>> cache_;
};

std::vector<SDFBlockMetadata> makeBlockMetadata(
    const ConstrainedSDFBinReader& reader) {
  std::vector<SDFBlockMetadata> out;
  out.reserve(reader.blocks().size());
  for (const ConstrainedSDFBinBlockRecord& block : reader.blocks()) {
    SDFBlockMetadata metadata;
    metadata.block_id = block.block_id;
    metadata.origin = block.bounds.min;
    metadata.local_min = block.bounds.min;
    metadata.local_max = block.bounds.max;
    metadata.resolution = {block.nx, block.ny, block.nz};
    metadata.compression_rank =
        std::max({block.rank_x, block.rank_y, block.rank_z});
    metadata.near_surface_error = reader.maxZeroSurfaceAbsError();
    metadata.max_reconstruction_error = block.actual_max_abs_error;
    out.push_back(metadata);
  }
  return out;
}

}  // namespace

const char* toString(ConstrainedSDFQueryStorage storage) {
  switch (storage) {
    case ConstrainedSDFQueryStorage::CompressedDirect:
      return "compressed-direct";
    case ConstrainedSDFQueryStorage::DecodedCache:
      return "decoded-cache";
  }
  return "compressed-direct";
}

ConstrainedSDFModel::ConstrainedSDFModel(
    ConstrainedSDFBinReader reader,
    ConstrainedSDFQueryStorage storage)
    : reader_(std::make_shared<ConstrainedSDFBinReader>(std::move(reader))),
      storage_(storage) {
  if (!reader_->valid()) {
    setValid(false);
    return;
  }
  setBoundingBox(reader_->bounds());
  setBlockMetadata(makeBlockMetadata(*reader_));
  SDFMetadata metadata;
  metadata.model_name = "constrained adaptive compressed SDF";
  metadata.format_name = ConstrainedSDFBin::magic();
  metadata.format_version = kConstrainedSDFBinFormatVersion;
  metadata.query_backend =
      std::string(backendName()) + " [" + toString(storage_) + "]";
  metadata.query_backend_available = true;
  metadata.final_block_count = static_cast<int>(reader_->blocks().size());
  metadata.active_block_count = metadata.final_block_count;
  metadata.near_surface_error = reader_->maxZeroSurfaceAbsError();
  std::size_t compressed_bytes = 0;
  for (const ConstrainedSDFBinBlockRecord& block : reader_->blocks()) {
    compressed_bytes += static_cast<std::size_t>(block.compressed_bytes);
    metadata.max_level = std::max(metadata.max_level, block.level);
    metadata.max_reconstruction_error = std::max(
        metadata.max_reconstruction_error, block.actual_max_abs_error);
  }
  metadata.total_low_rank_memory_mb =
      static_cast<double>(compressed_bytes) / (1024.0 * 1024.0);
  setMetadata(metadata);
  setMemoryFootprintBytes(compressed_bytes);
  setNativeHandle(
      std::make_shared<ConstrainedNativeHandle>(reader_, storage_));
  setValid(true);
}

std::shared_ptr<ConstrainedSDFModel> ConstrainedSDFModel::load(
    const std::filesystem::path& path,
    ConstrainedSDFQueryStorage storage) {
  ConstrainedSDFBinReport report;
  ConstrainedSDFBinReader reader = ConstrainedSDFBinReader::open(path, &report);
  if (!report.success) {
    throw std::runtime_error(report.error_message);
  }
  return std::make_shared<ConstrainedSDFModel>(std::move(reader), storage);
}

const char* ConstrainedSDFModel::backendName() {
  return "constrained block-local compressed SDF backend";
}

}  // namespace adasdf

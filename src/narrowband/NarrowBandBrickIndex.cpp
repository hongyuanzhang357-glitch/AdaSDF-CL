#include "adasdf/narrowband/NarrowBandBrickIndex.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>

namespace adasdf {
namespace {

bool containsPoint(const AABB& box, const Vector3& p) {
  const double eps = 1.0e-12;
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

int gridCount(int level) {
  if (level <= 0) {
    return 1;
  }
  if (level >= 20) {
    return 1 << 20;
  }
  return 1 << level;
}

int axisIndex(double value, double min_value, double max_value, int count) {
  if (count <= 1 || !(max_value > min_value)) {
    return 0;
  }
  const double u =
      std::clamp((value - min_value) / (max_value - min_value), 0.0, 1.0);
  return std::clamp(
      static_cast<int>(std::floor(u * static_cast<double>(count))),
      0,
      count - 1);
}

std::string dimKey(int nx, int ny, int nz) {
  return std::to_string(nx) + "x" + std::to_string(ny) + "x" +
         std::to_string(nz);
}

}  // namespace

bool NarrowBandBrickIndex::build(const AdaptiveBlockSDFModel& model) {
  return build(model.blockSet());
}

bool NarrowBandBrickIndex::build(const AdaptiveSDFBlockSet& blocks) {
  bounds_ = blocks.global_bounds;
  levels_desc_.clear();
  records_.clear();
  key_to_record_.clear();
  stats_ = {};
  valid_ = false;
  if (!bounds_.valid || blocks.blocks.empty()) {
    return false;
  }

  std::set<int, std::greater<int>> levels;
  records_.reserve(blocks.blocks.size());
  std::size_t data_offset = 0;
  for (std::size_t i = 0; i < blocks.blocks.size(); ++i) {
    const AdaptiveSDFBlock& block = blocks.blocks[i];
    NarrowBandBrickRecord record;
    record.brick_id = block.block_id;
    record.brick_level = block.level;
    record.bounds = block.bounds;
    record.tensor_nx = block.nx;
    record.tensor_ny = block.ny;
    record.tensor_nz = block.nz;
    record.data_offset = data_offset;
    record.cache_slot = records_.size();
    record.block = &block;
    record.spatial_key = keyForBlock(block);
    key_to_record_[record.spatial_key] = record.cache_slot;
    records_.push_back(record);
    levels.insert(block.level);
    ++stats_.block_count_by_level[block.level];
    ++stats_.tensor_dim_distribution[dimKey(block.nx, block.ny, block.nz)];
    data_offset += block.phi.size();
  }
  levels_desc_.assign(levels.begin(), levels.end());
  stats_.block_count = records_.size();
  valid_ = !records_.empty() && !levels_desc_.empty();
  return valid_;
}

bool NarrowBandBrickIndex::valid() const {
  return valid_;
}

const AABB& NarrowBandBrickIndex::bounds() const {
  return bounds_;
}

const NarrowBandBrickIndexStats& NarrowBandBrickIndex::stats() const {
  return stats_;
}

const std::vector<NarrowBandBrickRecord>& NarrowBandBrickIndex::records() const {
  return records_;
}

const NarrowBandBrickRecord* NarrowBandBrickIndex::find(
    const Vector3& point,
    NarrowBandBrickLookupStats* stats) const {
  if (stats != nullptr) {
    ++stats->block_lookup_count;
  }
  if (!valid_ || !containsPoint(bounds_, point)) {
    if (stats != nullptr) {
      ++stats->out_of_domain_count;
      ++stats->block_lookup_miss_count;
    }
    return nullptr;
  }

  for (int level : levels_desc_) {
    if (stats != nullptr) {
      ++stats->blocks_checked;
    }
    const std::uint64_t key = keyForPoint(level, point);
    const auto found = key_to_record_.find(key);
    if (found == key_to_record_.end()) {
      continue;
    }
    const NarrowBandBrickRecord& record = records_[found->second];
    if (containsPoint(record.bounds, point)) {
      return &record;
    }
  }
  if (stats != nullptr) {
    ++stats->block_lookup_miss_count;
  }
  return nullptr;
}

std::uint64_t NarrowBandBrickIndex::spatialKey(
    int level,
    int ix,
    int iy,
    int iz) {
  return (static_cast<std::uint64_t>(level & 0xffff) << 48) |
         (static_cast<std::uint64_t>(ix & 0xffff) << 32) |
         (static_cast<std::uint64_t>(iy & 0xffff) << 16) |
         static_cast<std::uint64_t>(iz & 0xffff);
}

std::uint64_t NarrowBandBrickIndex::keyForPoint(
    int level,
    const Vector3& point) const {
  const int count = gridCount(level);
  return spatialKey(
      level,
      axisIndex(point.x, bounds_.min.x, bounds_.max.x, count),
      axisIndex(point.y, bounds_.min.y, bounds_.max.y, count),
      axisIndex(point.z, bounds_.min.z, bounds_.max.z, count));
}

std::uint64_t NarrowBandBrickIndex::keyForBlock(
    const AdaptiveSDFBlock& block) const {
  const Vector3 center = (block.bounds.min + block.bounds.max) * 0.5;
  return keyForPoint(block.level, center);
}

}  // namespace adasdf


#include "adasdf/query/ExpandedSDF.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace adasdf {
namespace {

bool finite(const Vector3& v) {
  return v.allFinite();
}

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

double norm(const Vector3& v) {
  return std::sqrt(dot(v, v));
}

Vector3 normalizedOrFallback(const Vector3& value) {
  const double length = norm(value);
  if (!(length > 1.0e-12) || !value.allFinite()) {
    return {1.0, 0.0, 0.0};
  }
  return value / length;
}

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

bool blockValid(const ExpandedBlock& block) {
  if (!finite(block.min_corner) || !finite(block.max_corner)) {
    return false;
  }
  if (!(block.min_corner.x < block.max_corner.x) ||
      !(block.min_corner.y < block.max_corner.y) ||
      !(block.min_corner.z < block.max_corner.z)) {
    return false;
  }
  if (block.resolution_x < 2 || block.resolution_y < 2 ||
      block.resolution_z < 2) {
    return false;
  }
  const std::size_t expected =
      static_cast<std::size_t>(block.resolution_x) *
      static_cast<std::size_t>(block.resolution_y) *
      static_cast<std::size_t>(block.resolution_z);
  return block.values.size() == expected;
}

bool blockContains(const ExpandedBlock& block, const Vector3& p) {
  constexpr double eps = 1.0e-12;
  return p.x >= block.min_corner.x - eps && p.x <= block.max_corner.x + eps &&
         p.y >= block.min_corner.y - eps && p.y <= block.max_corner.y + eps &&
         p.z >= block.min_corner.z - eps && p.z <= block.max_corner.z + eps;
}

double blockCellSize(const ExpandedBlock& block) {
  const double hx = (block.max_corner.x - block.min_corner.x) /
                    static_cast<double>(std::max(1, block.resolution_x - 1));
  const double hy = (block.max_corner.y - block.min_corner.y) /
                    static_cast<double>(std::max(1, block.resolution_y - 1));
  const double hz = (block.max_corner.z - block.min_corner.z) /
                    static_cast<double>(std::max(1, block.resolution_z - 1));
  return std::min({hx, hy, hz});
}

Vector3 clampToBlock(const ExpandedBlock& block, const Vector3& p) {
  return {
      std::max(block.min_corner.x, std::min(block.max_corner.x, p.x)),
      std::max(block.min_corner.y, std::min(block.max_corner.y, p.y)),
      std::max(block.min_corner.z, std::min(block.max_corner.z, p.z))};
}

double sampleBlock(const ExpandedBlock& block, const Vector3& point, bool clamp) {
  if (!blockValid(block)) {
    throw std::runtime_error("ExpandedSDF block is invalid.");
  }
  if (!blockContains(block, point) && !clamp) {
    throw std::runtime_error("Point is outside the expanded SDF block.");
  }
  const Vector3 p = clamp ? clampToBlock(block, point) : point;

  const double sx = (p.x - block.min_corner.x) /
                    (block.max_corner.x - block.min_corner.x) *
                    static_cast<double>(block.resolution_x - 1);
  const double sy = (p.y - block.min_corner.y) /
                    (block.max_corner.y - block.min_corner.y) *
                    static_cast<double>(block.resolution_y - 1);
  const double sz = (p.z - block.min_corner.z) /
                    (block.max_corner.z - block.min_corner.z) *
                    static_cast<double>(block.resolution_z - 1);

  int i = static_cast<int>(std::floor(sx));
  int j = static_cast<int>(std::floor(sy));
  int k = static_cast<int>(std::floor(sz));
  i = std::max(0, std::min(i, block.resolution_x - 2));
  j = std::max(0, std::min(j, block.resolution_y - 2));
  k = std::max(0, std::min(k, block.resolution_z - 2));

  const double ax = std::min(1.0, std::max(0.0, sx - static_cast<double>(i)));
  const double ay = std::min(1.0, std::max(0.0, sy - static_cast<double>(j)));
  const double az = std::min(1.0, std::max(0.0, sz - static_cast<double>(k)));

  const int nx = block.resolution_x;
  const int ny = block.resolution_y;
  const auto v = [&](int di, int dj, int dk) {
    return block.values[valueIndex(i + di, j + dj, k + dk, nx, ny)];
  };

  const double c00 = (1.0 - ax) * v(0, 0, 0) + ax * v(1, 0, 0);
  const double c10 = (1.0 - ax) * v(0, 1, 0) + ax * v(1, 1, 0);
  const double c01 = (1.0 - ax) * v(0, 0, 1) + ax * v(1, 0, 1);
  const double c11 = (1.0 - ax) * v(0, 1, 1) + ax * v(1, 1, 1);
  const double c0 = (1.0 - ay) * c00 + ay * c10;
  const double c1 = (1.0 - ay) * c01 + ay * c11;
  return (1.0 - az) * c0 + az * c1;
}

double spacingForGradient(const ExpandedBlock& block) {
  const double hx = (block.max_corner.x - block.min_corner.x) /
                    static_cast<double>(std::max(1, block.resolution_x - 1));
  const double hy = (block.max_corner.y - block.min_corner.y) /
                    static_cast<double>(std::max(1, block.resolution_y - 1));
  const double hz = (block.max_corner.z - block.min_corner.z) /
                    static_cast<double>(std::max(1, block.resolution_z - 1));
  return 0.5 * std::min({hx, hy, hz});
}

const ExpandedBlock& blockForPoint(
    const std::vector<ExpandedBlock>& blocks,
    const Vector3& p,
    bool clamp) {
  const ExpandedBlock* best = nullptr;
  double best_cell_size = std::numeric_limits<double>::infinity();
  for (const ExpandedBlock& block : blocks) {
    if (blockContains(block, p)) {
      const double cell_size = blockCellSize(block);
      if (best == nullptr || cell_size < best_cell_size ||
          (cell_size == best_cell_size && block.block_id < best->block_id)) {
        best = &block;
        best_cell_size = cell_size;
      }
    }
  }
  if (best != nullptr) {
    return *best;
  }
  if (clamp && !blocks.empty()) {
    return blocks.front();
  }
  throw std::runtime_error("Point is outside the expanded SDF domain.");
}

}  // namespace

ExpandedSDF ExpandedSDF::globalDense(ExpandedBlock block) {
  if (block.block_id < 0) {
    block.block_id = 0;
  }
  if (!blockValid(block)) {
    throw std::runtime_error("Global expanded SDF block is invalid.");
  }
  ExpandedSDF expanded;
  expanded.layout_ = ExpandedSDFLayout::GlobalDense;
  expanded.block_ids_.push_back(block.block_id);
  expanded.blocks_.push_back(std::move(block));
  return expanded;
}

ExpandedSDF ExpandedSDF::blockDense(std::vector<ExpandedBlock> blocks) {
  if (blocks.empty()) {
    throw std::runtime_error("Block expanded SDF requires at least one block.");
  }
  std::sort(
      blocks.begin(),
      blocks.end(),
      [](const ExpandedBlock& a, const ExpandedBlock& b) {
        return a.block_id < b.block_id;
      });
  ExpandedSDF expanded;
  expanded.layout_ = ExpandedSDFLayout::BlockDense;
  for (const ExpandedBlock& block : blocks) {
    if (!blockValid(block)) {
      throw std::runtime_error("Block expanded SDF contains invalid block data.");
    }
    if (block.block_id < 0) {
      throw std::runtime_error("Block expanded SDF requires non-negative block ids.");
    }
    expanded.block_ids_.push_back(block.block_id);
  }
  std::sort(expanded.block_ids_.begin(), expanded.block_ids_.end());
  expanded.block_ids_.erase(
      std::unique(expanded.block_ids_.begin(), expanded.block_ids_.end()),
      expanded.block_ids_.end());
  expanded.blocks_ = std::move(blocks);
  return expanded;
}

bool ExpandedSDF::isValid() const {
  if (blocks_.empty()) {
    return false;
  }
  for (const ExpandedBlock& block : blocks_) {
    if (!blockValid(block)) {
      return false;
    }
  }
  return true;
}

bool ExpandedSDF::hasBlock(int block_id) const {
  return std::find(block_ids_.begin(), block_ids_.end(), block_id) !=
         block_ids_.end();
}

AABB ExpandedSDF::boundingBox() const {
  AABB bounds;
  for (const ExpandedBlock& block : blocks_) {
    if (!blockValid(block)) {
      continue;
    }
    if (!bounds.valid) {
      bounds.min = block.min_corner;
      bounds.max = block.max_corner;
      bounds.valid = true;
    } else {
      bounds.min.x = std::min(bounds.min.x, block.min_corner.x);
      bounds.min.y = std::min(bounds.min.y, block.min_corner.y);
      bounds.min.z = std::min(bounds.min.z, block.min_corner.z);
      bounds.max.x = std::max(bounds.max.x, block.max_corner.x);
      bounds.max.y = std::max(bounds.max.y, block.max_corner.y);
      bounds.max.z = std::max(bounds.max.z, block.max_corner.z);
    }
  }
  return bounds;
}

std::size_t ExpandedSDF::memoryFootprintBytes() const {
  std::size_t bytes = sizeof(*this) + block_ids_.size() * sizeof(int);
  for (const ExpandedBlock& block : blocks_) {
    bytes += sizeof(ExpandedBlock);
    bytes += block.values.size() * sizeof(double);
  }
  return bytes;
}

bool ExpandedSDF::contains(const Vector3& p) const {
  for (const ExpandedBlock& block : blocks_) {
    if (blockContains(block, p)) {
      return true;
    }
  }
  return false;
}

double ExpandedSDF::sampleDistance(const Vector3& p) const {
  return sampleBlock(
      blockForPoint(blocks_, p, policy_.clamp_outside_expanded_domain),
      p,
      policy_.clamp_outside_expanded_domain);
}

Vector3 ExpandedSDF::sampleGradient(const Vector3& p) const {
  const ExpandedBlock& block =
      blockForPoint(blocks_, p, policy_.clamp_outside_expanded_domain);
  const double h = spacingForGradient(block);
  if (!(h > 0.0) || !std::isfinite(h)) {
    return {1.0, 0.0, 0.0};
  }

  const Vector3 dx{h, 0.0, 0.0};
  const Vector3 dy{0.0, h, 0.0};
  const Vector3 dz{0.0, 0.0, h};
  try {
    const double gx =
        (sampleDistance(p + dx) - sampleDistance(p - dx)) / (2.0 * h);
    const double gy =
        (sampleDistance(p + dy) - sampleDistance(p - dy)) / (2.0 * h);
    const double gz =
        (sampleDistance(p + dz) - sampleDistance(p - dz)) / (2.0 * h);
    return normalizedOrFallback({gx, gy, gz});
  } catch (const std::exception&) {
    return normalizedOrFallback({
        sampleDistance(p) - sampleBlock(
                                block,
                                {std::max(block.min_corner.x, p.x - h), p.y, p.z},
                                true),
        sampleDistance(p) - sampleBlock(
                                block,
                                {p.x, std::max(block.min_corner.y, p.y - h), p.z},
                                true),
        sampleDistance(p) - sampleBlock(
                                block,
                                {p.x, p.y, std::max(block.min_corner.z, p.z - h)},
                                true)});
  }
}

std::string ExpandedSDF::description() const {
  std::ostringstream out;
  out << toString(layout_) << " blocks=" << blocks_.size()
      << " memory_bytes=" << memoryFootprintBytes();
  return out.str();
}

const char* toString(ExpandedSDFLayout layout) {
  switch (layout) {
    case ExpandedSDFLayout::GlobalDense:
      return "global_dense";
    case ExpandedSDFLayout::BlockDense:
      return "block_dense";
  }
  return "unknown";
}

}  // namespace adasdf

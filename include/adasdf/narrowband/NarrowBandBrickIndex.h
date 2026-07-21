#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"

namespace adasdf {

struct NarrowBandBrickRecord {
  int brick_id = -1;
  int brick_level = 0;
  AABB bounds;
  int tensor_nx = 0;
  int tensor_ny = 0;
  int tensor_nz = 0;
  std::size_t data_offset = 0;
  std::uint64_t spatial_key = 0;
  std::size_t cache_slot = 0;
  const AdaptiveSDFBlock* block = nullptr;
};

struct NarrowBandBrickIndexStats {
  std::size_t block_count = 0;
  std::map<int, std::size_t> block_count_by_level;
  std::map<std::string, std::size_t> tensor_dim_distribution;
};

struct NarrowBandBrickLookupStats {
  std::size_t block_lookup_count = 0;
  std::size_t block_lookup_miss_count = 0;
  std::size_t out_of_domain_count = 0;
  std::size_t blocks_checked = 0;
};

class NarrowBandBrickIndex {
 public:
  bool build(const AdaptiveBlockSDFModel& model);
  bool build(const AdaptiveSDFBlockSet& blocks);

  bool valid() const;
  const AABB& bounds() const;
  const NarrowBandBrickIndexStats& stats() const;
  const std::vector<NarrowBandBrickRecord>& records() const;

  const NarrowBandBrickRecord* find(
      const Vector3& point,
      NarrowBandBrickLookupStats* stats = nullptr) const;

  static std::uint64_t spatialKey(int level, int ix, int iy, int iz);

 private:
  std::uint64_t keyForPoint(int level, const Vector3& point) const;
  std::uint64_t keyForBlock(const AdaptiveSDFBlock& block) const;

  AABB bounds_;
  std::vector<int> levels_desc_;
  std::vector<NarrowBandBrickRecord> records_;
  std::unordered_map<std::uint64_t, std::size_t> key_to_record_;
  NarrowBandBrickIndexStats stats_;
  bool valid_ = false;
};

}  // namespace adasdf


#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "adasdf/sampling/ContactBandMarker.h"

namespace adasdf {

struct MarkerCellKey {
  int block_id = -1;
  int cell_i = 0;
  int cell_j = 0;
  int cell_k = 0;
  int level = 0;
  std::uint64_t marker_config_id = 0;

  bool operator==(const MarkerCellKey& other) const noexcept;
};

struct MarkerCellKeyHash {
  std::size_t operator()(const MarkerCellKey& key) const noexcept;
};

struct ContactBandMarkerCacheStats {
  std::size_t candidate_lookup_count = 0;
  std::size_t candidate_hit_count = 0;
  std::size_t candidate_miss_count = 0;
  std::size_t candidate_insert_count = 0;
  std::size_t decision_lookup_count = 0;
  std::size_t decision_hit_count = 0;
  std::size_t decision_miss_count = 0;
  std::size_t decision_insert_count = 0;
  std::size_t box_triangle_distance_saved = 0;
  double marker_cache_time_ms = 0.0;
};

std::uint64_t contactBandMarkerConfigId(const ContactBandOptions& options);

class ContactBandMarkerCache {
 public:
  bool findCandidateTriangles(
      const MarkerCellKey& key,
      std::vector<int>* out) const;
  void insertCandidateTriangles(
      const MarkerCellKey& key,
      const std::vector<int>& tri_ids);

  bool findCellDecision(const MarkerCellKey& key, bool* is_contact_cell) const;
  void insertCellDecision(const MarkerCellKey& key, bool is_contact_cell);

  const ContactBandMarkerCacheStats& stats() const;
  ContactBandMarkerCacheStats snapshotStats() const;
  void clear();

 private:
  mutable std::mutex mutex_;
  mutable ContactBandMarkerCacheStats stats_;
  std::unordered_map<MarkerCellKey, std::vector<int>, MarkerCellKeyHash>
      candidate_triangles_;
  std::unordered_map<MarkerCellKey, bool, MarkerCellKeyHash> decisions_;
};

}  // namespace adasdf

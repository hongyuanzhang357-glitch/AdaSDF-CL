#include "adasdf/cache/ContactBandMarkerCache.h"

#include <chrono>
#include <cstring>

namespace adasdf {

bool MarkerCellKey::operator==(const MarkerCellKey& other) const noexcept {
  return block_id == other.block_id && cell_i == other.cell_i &&
         cell_j == other.cell_j && cell_k == other.cell_k &&
         level == other.level &&
         marker_config_id == other.marker_config_id;
}

namespace {

std::size_t mix(std::size_t seed, std::uint64_t value) {
  const std::size_t x = static_cast<std::size_t>(value);
  seed ^= x + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
  return seed;
}

std::uint64_t bitsOf(double value) {
  std::uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "unexpected double size");
  std::memcpy(&bits, &value, sizeof(value));
  return bits;
}

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

}  // namespace

std::size_t MarkerCellKeyHash::operator()(
    const MarkerCellKey& key) const noexcept {
  std::size_t seed = 0;
  seed = mix(seed, static_cast<std::uint64_t>(key.block_id));
  seed = mix(seed, static_cast<std::uint64_t>(key.cell_i));
  seed = mix(seed, static_cast<std::uint64_t>(key.cell_j));
  seed = mix(seed, static_cast<std::uint64_t>(key.cell_k));
  seed = mix(seed, static_cast<std::uint64_t>(key.level));
  seed = mix(seed, key.marker_config_id);
  return seed;
}

std::uint64_t contactBandMarkerConfigId(const ContactBandOptions& options) {
  std::size_t seed = 0;
  seed = mix(seed, bitsOf(options.contact_band_width));
  seed = mix(seed, static_cast<std::uint64_t>(options.contact_band_layers));
  seed = mix(seed, static_cast<std::uint64_t>(options.halo_exact_layers));
  seed = mix(seed, static_cast<std::uint64_t>(options.marker_mode));
  seed = mix(seed, bitsOf(options.marker_safety_factor));
  seed = mix(seed, bitsOf(options.marker_cell_size_factor));
  seed = mix(seed, bitsOf(options.marker_min_band));
  seed = mix(seed, bitsOf(options.marker_max_band));
  seed = mix(seed, options.disable_global_halo ? 1 : 0);
  seed = mix(seed, options.local_halo_only ? 1 : 0);
  return static_cast<std::uint64_t>(seed);
}

bool ContactBandMarkerCache::findCandidateTriangles(
    const MarkerCellKey& key,
    std::vector<int>* out) const {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  ++stats_.candidate_lookup_count;
  const auto it = candidate_triangles_.find(key);
  if (it == candidate_triangles_.end()) {
    ++stats_.candidate_miss_count;
    stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
    return false;
  }
  ++stats_.candidate_hit_count;
  if (out != nullptr) {
    *out = it->second;
  }
  stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
  return true;
}

void ContactBandMarkerCache::insertCandidateTriangles(
    const MarkerCellKey& key,
    const std::vector<int>& tri_ids) {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  const auto inserted = candidate_triangles_.emplace(key, tri_ids);
  if (!inserted.second) {
    inserted.first->second = tri_ids;
  } else {
    ++stats_.candidate_insert_count;
  }
  stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
}

bool ContactBandMarkerCache::findCellDecision(
    const MarkerCellKey& key,
    bool* is_contact_cell) const {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  ++stats_.decision_lookup_count;
  const auto it = decisions_.find(key);
  if (it == decisions_.end()) {
    ++stats_.decision_miss_count;
    stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
    return false;
  }
  ++stats_.decision_hit_count;
  ++stats_.box_triangle_distance_saved;
  if (is_contact_cell != nullptr) {
    *is_contact_cell = it->second;
  }
  stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
  return true;
}

void ContactBandMarkerCache::insertCellDecision(
    const MarkerCellKey& key,
    bool is_contact_cell) {
  const auto t0 = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  const auto inserted = decisions_.emplace(key, is_contact_cell);
  if (!inserted.second) {
    inserted.first->second = is_contact_cell;
  } else {
    ++stats_.decision_insert_count;
  }
  stats_.marker_cache_time_ms += elapsedMs(t0, std::chrono::steady_clock::now());
}

const ContactBandMarkerCacheStats& ContactBandMarkerCache::stats() const {
  return stats_;
}

ContactBandMarkerCacheStats ContactBandMarkerCache::snapshotStats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return stats_;
}

void ContactBandMarkerCache::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  candidate_triangles_.clear();
  decisions_.clear();
  stats_ = {};
}

}  // namespace adasdf

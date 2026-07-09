#include <adasdf/adasdf.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  adasdf::ContactBandOptions options;
  options.contact_band_width = 0.001;
  options.marker_cell_size_factor = 0.5;
  options.marker_safety_factor = 1.0;
  const std::uint64_t config_a = adasdf::contactBandMarkerConfigId(options);
  options.contact_band_width = 0.002;
  const std::uint64_t config_b = adasdf::contactBandMarkerConfigId(options);
  if (config_a == config_b) {
    std::cerr << "marker config id should change with marker parameters\n";
    return 1;
  }

  adasdf::ContactBandMarkerCache cache;
  adasdf::MarkerCellKey key;
  key.block_id = 4;
  key.cell_i = 1;
  key.cell_j = 2;
  key.cell_k = 3;
  key.level = 2;
  key.marker_config_id = config_a;

  bool is_contact = false;
  if (cache.findCellDecision(key, &is_contact)) {
    std::cerr << "empty marker decision cache should miss\n";
    return 1;
  }
  cache.insertCellDecision(key, true);
  if (!cache.findCellDecision(key, &is_contact) || !is_contact) {
    std::cerr << "marker decision cache failed to reuse decision\n";
    return 1;
  }
  std::vector<int> tri_ids;
  if (cache.findCandidateTriangles(key, &tri_ids)) {
    std::cerr << "empty marker candidate cache should miss\n";
    return 1;
  }
  cache.insertCandidateTriangles(key, {1, 2, 3});
  if (!cache.findCandidateTriangles(key, &tri_ids) || tri_ids.size() != 3) {
    std::cerr << "marker candidate cache failed to reuse triangles\n";
    return 1;
  }

  key.marker_config_id = config_b;
  if (cache.findCellDecision(key, &is_contact) ||
      cache.findCandidateTriangles(key, &tri_ids)) {
    std::cerr << "marker cache reused an entry across parameter configs\n";
    return 1;
  }
  const adasdf::ContactBandMarkerCacheStats stats = cache.snapshotStats();
  if (stats.decision_hit_count != 1 || stats.decision_miss_count != 2 ||
      stats.candidate_hit_count != 1 || stats.candidate_miss_count != 2 ||
      stats.box_triangle_distance_saved != 1) {
    std::cerr << "unexpected marker cache stats\n";
    return 1;
  }
  return 0;
}

#include <adasdf/adasdf.h>

#include <iostream>
#include <string>

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  adasdf::BuildProfiler profiler(true);
  profiler.setToolName("test_build_cache_profile_fields");
  profiler.timings.cache_lookup_time_ms = 1.5;
  profiler.timings.cache_insert_time_ms = 0.5;
  profiler.timings.deduplication_time_ms = 0.25;
  profiler.timings.marker_cache_time_ms = 0.125;
  profiler.counters.sample_cache_enabled = true;
  profiler.counters.sample_cache_scope = "block";
  profiler.counters.sample_cache_entries = 3;
  profiler.counters.sample_cache_hits = 2;
  profiler.counters.sample_cache_misses = 1;
  profiler.counters.sample_cache_hit_rate = 2.0 / 3.0;
  profiler.counters.distance_cache_hits = 2;
  profiler.counters.distance_cache_misses = 1;
  profiler.counters.sign_cache_hits = 2;
  profiler.counters.sign_cache_misses = 1;
  profiler.counters.corner_cache_hits = 4;
  profiler.counters.corner_cache_misses = 2;
  profiler.counters.block_point_duplicate_count = 5;
  profiler.counters.marker_candidate_cache_hits = 6;
  profiler.counters.marker_candidate_cache_misses = 7;
  profiler.counters.marker_decision_cache_hits = 8;
  profiler.counters.marker_decision_cache_misses = 9;
  profiler.counters.distance_queries_saved = 10;
  profiler.counters.sign_queries_saved = 11;
  profiler.counters.box_triangle_distance_saved = 12;
  profiler.counters.cache_memory_estimate_bytes = 13;

  const std::string json = adasdf::BuildProfileJsonWriter::toJson(profiler);
  for (const std::string& field : {
           "\"cache_lookup_time_ms\"",
           "\"cache_insert_time_ms\"",
           "\"deduplication_time_ms\"",
           "\"marker_cache_time_ms\"",
           "\"sample_cache_enabled\"",
           "\"sample_cache_scope\"",
           "\"sample_cache_entries\"",
           "\"sample_cache_hits\"",
           "\"sample_cache_misses\"",
           "\"distance_cache_hits\"",
           "\"distance_cache_misses\"",
           "\"sign_cache_hits\"",
           "\"sign_cache_misses\"",
           "\"corner_cache_hits\"",
           "\"corner_cache_misses\"",
           "\"block_point_duplicate_count\"",
           "\"marker_candidate_cache_hits\"",
           "\"marker_candidate_cache_misses\"",
           "\"marker_decision_cache_hits\"",
           "\"marker_decision_cache_misses\"",
           "\"distance_queries_saved\"",
           "\"sign_queries_saved\"",
           "\"box_triangle_distance_saved\"",
           "\"cache_memory_estimate_bytes\""}) {
    if (!contains(json, field)) {
      std::cerr << "profile JSON missing field " << field << "\n";
      return 1;
    }
  }
  return 0;
}

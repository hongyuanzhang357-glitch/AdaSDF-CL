#include "adasdf/cache/BuildCacheReportWriter.h"

#include <sstream>

namespace adasdf {

std::string BuildCacheReportWriter::toMarkdown(const BuildCacheStats& stats) {
  std::ostringstream out;
  out << "# Build Cache Stats\n\n";
  out << "- Sample cache enabled: "
      << (stats.sample_cache_enabled ? "yes" : "no") << "\n";
  out << "- Sample cache scope: " << toString(stats.sample_cache_scope) << "\n";
  out << "- Sample cache entries: " << stats.sample_cache_entries << "\n";
  out << "- Sample cache hits: " << stats.sample_cache_hits << "\n";
  out << "- Sample cache misses: " << stats.sample_cache_misses << "\n";
  out << "- Sample cache hit rate: " << stats.sample_cache_hit_rate << "\n";
  out << "- Distance queries saved: " << stats.distance_queries_saved << "\n";
  out << "- Sign queries saved: " << stats.sign_queries_saved << "\n";
  out << "- Duplicate points: " << stats.block_point_duplicate_count << "\n";
  out << "- Marker candidate hits: "
      << stats.marker_candidate_cache_hits << "\n";
  out << "- Marker decision hits: " << stats.marker_decision_cache_hits << "\n";
  return out.str();
}

std::string BuildCacheReportWriter::csvHeader() {
  return "sample_cache_enabled,sample_cache_scope,sample_cache_entries,"
         "sample_cache_hits,sample_cache_misses,sample_cache_hit_rate,"
         "distance_queries_saved,sign_queries_saved,"
         "block_point_duplicate_count,marker_candidate_cache_hits,"
         "marker_candidate_cache_misses,marker_decision_cache_hits,"
         "marker_decision_cache_misses,box_triangle_distance_saved,"
         "cache_memory_estimate_bytes";
}

std::string BuildCacheReportWriter::csvRow(const BuildCacheStats& stats) {
  std::ostringstream out;
  out << (stats.sample_cache_enabled ? "true" : "false") << ","
      << toString(stats.sample_cache_scope) << ","
      << stats.sample_cache_entries << ","
      << stats.sample_cache_hits << ","
      << stats.sample_cache_misses << ","
      << stats.sample_cache_hit_rate << ","
      << stats.distance_queries_saved << ","
      << stats.sign_queries_saved << ","
      << stats.block_point_duplicate_count << ","
      << stats.marker_candidate_cache_hits << ","
      << stats.marker_candidate_cache_misses << ","
      << stats.marker_decision_cache_hits << ","
      << stats.marker_decision_cache_misses << ","
      << stats.box_triangle_distance_saved << ","
      << stats.cache_memory_estimate_bytes;
  return out.str();
}

}  // namespace adasdf

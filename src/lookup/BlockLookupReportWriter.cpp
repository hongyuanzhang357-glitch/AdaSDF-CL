#include "adasdf/lookup/BlockLookupReportWriter.h"

#include <sstream>

namespace adasdf {

std::string BlockLookupReportWriter::csvHeader() {
  return
      "case_id,model_type,sample_count,block_count,active_block_count,"
      "lookup_mode,cache_lookup_mode,index_build_time_ms,"
      "cache_map_build_time_ms,linear_query_time_ms,hash_query_time_ms,"
      "morton_query_time_ms,spatial_hash_query_time_ms,speedup_vs_linear,"
      "lookup_result_mismatch_count,phi_mismatch_count,max_abs_phi_diff,"
      "rms_phi_diff,p95_phi_diff,cache_hit_count,cache_miss_count,"
      "cache_hit_rate,linear_fallback_count,missed_lookup_count,"
      "quality_passed,performance_claim_allowed\n";
}

std::string BlockLookupReportWriter::markdownSummary(
    const std::string& case_id,
    const BlockLookupDiagnostics& diagnostics) {
  std::ostringstream out;
  out << "# Block Lookup Benchmark\n\n";
  out << "- Case id: " << case_id << "\n";
  out << "- Blocks: " << diagnostics.lookup_stats.block_count << "\n";
  out << "- Buckets: " << diagnostics.lookup_stats.bucket_count << "\n";
  out << "- Max bucket size: " << diagnostics.lookup_stats.max_bucket_size
      << "\n";
  out << "- Lookup mismatches: "
      << diagnostics.lookup_result_mismatch_count << "\n";
  out << "- Phi mismatches: " << diagnostics.phi_mismatch_count << "\n";
  out << "- Max abs phi diff: " << diagnostics.max_abs_phi_diff << "\n";
  out << "- Cache hit rate: " << diagnostics.cache_stats.hit_rate << "\n";
  out << "- Linear fallbacks: "
      << diagnostics.lookup_stats.linear_fallback_count << "\n";
  out << "- Quality passed: "
      << (diagnostics.quality_passed ? "yes" : "no") << "\n";
  out << "- Performance claim allowed: "
      << (diagnostics.performance_claim_allowed ? "yes" : "no") << "\n";
  return out.str();
}

}  // namespace adasdf

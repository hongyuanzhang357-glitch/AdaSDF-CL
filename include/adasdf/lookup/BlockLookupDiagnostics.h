#pragma once

#include <cstddef>

#include "adasdf/lookup/ActiveBlockHashMap.h"
#include "adasdf/lookup/BlockLookupIndex.h"

namespace adasdf {

struct BlockLookupDiagnostics {
  BlockLookupIndexStats lookup_stats;
  ActiveBlockHashMapStats cache_stats;
  std::size_t lookup_result_mismatch_count = 0;
  std::size_t phi_mismatch_count = 0;
  double max_abs_phi_diff = 0.0;
  double rms_phi_diff = 0.0;
  double p95_phi_diff = 0.0;
  bool quality_passed = false;
  bool performance_claim_allowed = false;
};

}  // namespace adasdf

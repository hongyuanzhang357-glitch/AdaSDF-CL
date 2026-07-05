#include "adasdf/sampling/ContactBandDiagnostics.h"

namespace adasdf {

void finalizeContactBandDiagnostics(ContactBandDiagnostics* diagnostics) {
  if (diagnostics == nullptr) {
    return;
  }
  if (diagnostics->total_block_count > 0) {
    diagnostics->contact_band_block_ratio =
        static_cast<double>(diagnostics->contact_band_block_count) /
        static_cast<double>(diagnostics->total_block_count);
  }
  if (diagnostics->total_node_count > 0) {
    diagnostics->exact_node_ratio =
        static_cast<double>(diagnostics->exact_node_count) /
        static_cast<double>(diagnostics->total_node_count);
    diagnostics->predicted_node_ratio =
        static_cast<double>(diagnostics->predicted_node_count) /
        static_cast<double>(diagnostics->total_node_count);
    diagnostics->exact_sample_reduction_ratio =
        1.0 - diagnostics->exact_node_ratio;
  }
  if (diagnostics->total_node_count > 0) {
    diagnostics->sign_query_reduction_ratio =
        1.0 - static_cast<double>(diagnostics->sign_query_count) /
                  static_cast<double>(diagnostics->total_node_count);
  }
  if (diagnostics->candidate_triangle_aabb_overlap_count > 0) {
    diagnostics->overmark_ratio_estimate =
        static_cast<double>(diagnostics->distance_rejected_cell_count) /
        static_cast<double>(diagnostics->candidate_triangle_aabb_overlap_count);
  }
}

}  // namespace adasdf

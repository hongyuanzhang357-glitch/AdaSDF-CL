#include "adasdf/generation/AdaptiveBlockSDFBuildReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "adasdf/acceleration/BuildAccelerationReport.h"

namespace adasdf {
namespace {

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string escaped(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

}  // namespace

std::string AdaptiveBlockSDFBuildReportWriter::toMarkdown(
    const AdaptiveBlockSDFBuildReport& report) {
  std::ostringstream out;
  out << "# Adaptive Block SDF Build Report\n\n";
  out << "## Summary\n\n";
  out << "- Success: " << yesNo(report.success) << "\n";
  if (!report.error_message.empty()) {
    out << "- Error: " << report.error_message << "\n";
  }
  out << "- Signed distance: " << yesNo(report.signed_distance) << "\n";
  out << "- Watertight: " << yesNo(report.watertight) << "\n";
  out << "- Used cleanup: " << yesNo(report.used_cleanup) << "\n";
  out << "- Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1\n\n";

  out << "## Mesh Diagnostics\n\n";
  out << "- Vertices: " << report.diagnostics.vertex_count << "\n";
  out << "- Triangles: " << report.diagnostics.triangle_count << "\n";
  out << "- Boundary edges: " << report.diagnostics.boundary_edge_count << "\n";
  out << "- Non-manifold edges: "
      << report.diagnostics.non_manifold_edge_count << "\n";
  out << "- Degenerate triangles: "
      << report.diagnostics.degenerate_triangle_count << "\n";
  out << "- Duplicate triangles: "
      << report.diagnostics.duplicate_triangle_count << "\n\n";

  out << "## Readiness\n\n";
  out << "- Level: " << toString(report.readiness.level) << "\n";
  out << "- Score: " << report.readiness.score << " / 100\n";
  out << "- Summary: " << report.readiness.summary << "\n\n";

  out << "## Octree Parameters\n\n";
  out << "- Min level: " << report.min_octree_level << "\n";
  out << "- Max level: " << report.max_octree_level << "\n";
  out << "- Max level used: " << report.max_octree_level_used << "\n\n";

  out << "## Octree Stats\n\n";
  out << "- Nodes: " << report.octree_node_count << "\n";
  out << "- Leaves: " << report.octree_leaf_count << "\n";
  out << "- Near-surface leaves: "
      << report.octree_report.near_surface_leaf_count << "\n\n";

  out << "## Block Stats\n\n";
  out << "- Block count: " << report.block_count << "\n";
  out << "- Near-surface blocks: " << report.near_surface_block_count << "\n";
  out << "- Block resolution: " << report.block_resolution << "\n";
  out << "- Block storage: dense phi values per adaptive leaf block\n\n";

  out << "## Memory\n\n";
  out << "- Memory bytes: " << report.memory_bytes << "\n\n";

  out << "## Build Time\n\n";
  out << "- Sampling time ms: " << report.sampling_time_ms << "\n";
  out << "- Build time ms: " << report.build_time_ms << "\n\n";

  out << adasdf::toMarkdown(report.acceleration_stats) << "\n\n";

  out << "## Hierarchical Sampling\n\n";
  out << "- Enabled blocks sampled: "
      << report.hierarchical_sampling.block_count << "\n";
  out << "- Exact blocks: "
      << report.hierarchical_sampling.exact_block_count << "\n";
  out << "- Predicted blocks: "
      << report.hierarchical_sampling.predicted_block_count << "\n";
  out << "- Accepted prediction blocks: "
      << report.hierarchical_sampling.accepted_prediction_block_count << "\n";
  out << "- Fallback exact blocks: "
      << report.hierarchical_sampling.fallback_exact_block_count << "\n";
  out << "- Exact sample count: "
      << report.hierarchical_sampling.exact_sample_count << "\n";
  out << "- Predicted sample count: "
      << report.hierarchical_sampling.predicted_sample_count << "\n";
  out << "- Quality check sample count: "
      << report.hierarchical_sampling.quality_check_sample_count << "\n";
  out << "- Exact sampling time ms: "
      << report.hierarchical_sampling.exact_sampling_time_ms << "\n";
  out << "- Prediction time ms: "
      << report.hierarchical_sampling.prediction_time_ms << "\n";
  out << "- Quality check time ms: "
      << report.hierarchical_sampling.quality_check_time_ms << "\n";
  out << "- Speedup vs exact estimate: "
      << report.hierarchical_sampling.speedup_vs_exact_estimate << "\n\n";

  out << "## Contact-Band Sampling\n\n";
  out << "- Total blocks: "
      << report.contact_band_sampling.total_block_count << "\n";
  out << "- Contact-band blocks: "
      << report.contact_band_sampling.contact_band_block_count << "\n";
  out << "- Far-field blocks: "
      << report.contact_band_sampling.far_field_block_count << "\n";
  out << "- Exact nodes: "
      << report.contact_band_sampling.exact_node_count << "\n";
  out << "- Predicted nodes: "
      << report.contact_band_sampling.predicted_node_count << "\n";
  out << "- Coarse samples: "
      << report.contact_band_sampling.coarse_sample_count << "\n";
  out << "- Exact node ratio: "
      << report.contact_band_sampling.exact_node_ratio << "\n";
  out << "- Exact sample reduction ratio: "
      << report.contact_band_sampling.exact_sample_reduction_ratio << "\n";
  out << "- Sign query reduction ratio: "
      << report.contact_band_sampling.sign_query_reduction_ratio << "\n\n";

  out << "## Warnings\n\n";
  if (report.warnings.empty()) {
    out << "- none\n\n";
  } else {
    for (const std::string& warning : report.warnings) {
      out << "- " << warning << "\n";
    }
    out << "\n";
  }

  out << "## Limitations\n\n";
  out << "- Brute-force triangle distance remains the default reference path.\n";
  out << "- BVH acceleration is an optional CPU path for adaptive block sampling.\n";
  out << "- This report describes dense adaptive block output.\n";
  out << "- Matrix-SVD compression is available through "
         "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1.\n";
  out << "- Tucker/HOSVD compression is not implemented in v1.8.0-alpha.\n";
  out << "- GPU-native compressed adaptive query is planned work.\n\n";

  out << "## Next Steps\n\n";
  out << "- Use adasdf_build_compressed_sdf or "
         "adasdf_compress_adaptive_sdf for low-rank compressed output.\n";
  out << "- Use adasdf_recommend_build for deterministic build recommendation.\n";
  return out.str();
}

std::string AdaptiveBlockSDFBuildReportWriter::toJson(
    const AdaptiveBlockSDFBuildReport& report) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"success\": " << (report.success ? "true" : "false") << ",\n";
  out << "  \"error_message\": \"" << escaped(report.error_message) << "\",\n";
  out << "  \"format\": \"ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1\",\n";
  out << "  \"signed_distance\": "
      << (report.signed_distance ? "true" : "false") << ",\n";
  out << "  \"watertight\": " << (report.watertight ? "true" : "false") << ",\n";
  out << "  \"used_cleanup\": " << (report.used_cleanup ? "true" : "false") << ",\n";
  out << "  \"min_octree_level\": " << report.min_octree_level << ",\n";
  out << "  \"max_octree_level\": " << report.max_octree_level << ",\n";
  out << "  \"max_octree_level_used\": " << report.max_octree_level_used << ",\n";
  out << "  \"block_resolution\": " << report.block_resolution << ",\n";
  out << "  \"octree_node_count\": " << report.octree_node_count << ",\n";
  out << "  \"octree_leaf_count\": " << report.octree_leaf_count << ",\n";
  out << "  \"block_count\": " << report.block_count << ",\n";
  out << "  \"near_surface_block_count\": "
      << report.near_surface_block_count << ",\n";
  out << "  \"memory_bytes\": " << report.memory_bytes << ",\n";
  out << "  \"sampling_time_ms\": " << report.sampling_time_ms << ",\n";
  out << "  \"build_time_ms\": " << report.build_time_ms << ",\n";
  out << "  \"acceleration\": " << adasdf::toJson(report.acceleration_stats)
      << ",\n";
  out << "  \"hierarchical_sampling\": {\n";
  out << "    \"block_count\": "
      << report.hierarchical_sampling.block_count << ",\n";
  out << "    \"exact_block_count\": "
      << report.hierarchical_sampling.exact_block_count << ",\n";
  out << "    \"predicted_block_count\": "
      << report.hierarchical_sampling.predicted_block_count << ",\n";
  out << "    \"accepted_prediction_block_count\": "
      << report.hierarchical_sampling.accepted_prediction_block_count
      << ",\n";
  out << "    \"fallback_exact_block_count\": "
      << report.hierarchical_sampling.fallback_exact_block_count << ",\n";
  out << "    \"exact_sample_count\": "
      << report.hierarchical_sampling.exact_sample_count << ",\n";
  out << "    \"predicted_sample_count\": "
      << report.hierarchical_sampling.predicted_sample_count << ",\n";
  out << "    \"quality_check_sample_count\": "
      << report.hierarchical_sampling.quality_check_sample_count << ",\n";
  out << "    \"exact_sampling_time_ms\": "
      << report.hierarchical_sampling.exact_sampling_time_ms << ",\n";
  out << "    \"prediction_time_ms\": "
      << report.hierarchical_sampling.prediction_time_ms << ",\n";
  out << "    \"quality_check_time_ms\": "
      << report.hierarchical_sampling.quality_check_time_ms << ",\n";
  out << "    \"total_time_ms\": "
      << report.hierarchical_sampling.total_time_ms << ",\n";
  out << "    \"speedup_vs_exact_estimate\": "
      << report.hierarchical_sampling.speedup_vs_exact_estimate << "\n";
  out << "  },\n";
  out << "  \"contact_band_sampling\": {\n";
  out << "    \"total_block_count\": "
      << report.contact_band_sampling.total_block_count << ",\n";
  out << "    \"contact_band_block_count\": "
      << report.contact_band_sampling.contact_band_block_count << ",\n";
  out << "    \"far_field_block_count\": "
      << report.contact_band_sampling.far_field_block_count << ",\n";
  out << "    \"total_node_count\": "
      << report.contact_band_sampling.total_node_count << ",\n";
  out << "    \"exact_node_count\": "
      << report.contact_band_sampling.exact_node_count << ",\n";
  out << "    \"predicted_node_count\": "
      << report.contact_band_sampling.predicted_node_count << ",\n";
  out << "    \"far_field_node_count\": "
      << report.contact_band_sampling.far_field_node_count << ",\n";
  out << "    \"coarse_sample_count\": "
      << report.contact_band_sampling.coarse_sample_count << ",\n";
  out << "    \"exact_node_ratio\": "
      << report.contact_band_sampling.exact_node_ratio << ",\n";
  out << "    \"exact_sample_reduction_ratio\": "
      << report.contact_band_sampling.exact_sample_reduction_ratio << ",\n";
  out << "    \"distance_query_count\": "
      << report.contact_band_sampling.distance_query_count << ",\n";
  out << "    \"sign_query_count\": "
      << report.contact_band_sampling.sign_query_count << ",\n";
  out << "    \"sign_query_reduction_ratio\": "
      << report.contact_band_sampling.sign_query_reduction_ratio << "\n";
  out << "  },\n";
  out << "  \"low_rank_compression\": \"available via ADASDF_COMPRESSED_BLOCK_SDFBIN_V1\",\n";
  out << "  \"warnings\": [";
  for (std::size_t i = 0; i < report.warnings.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << "\"" << escaped(report.warnings[i]) << "\"";
  }
  out << "]\n";
  out << "}\n";
  return out.str();
}

void AdaptiveBlockSDFBuildReportWriter::writeMarkdown(
    const std::string& path_string,
    const AdaptiveBlockSDFBuildReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toMarkdown(report);
}

void AdaptiveBlockSDFBuildReportWriter::writeJson(
    const std::string& path_string,
    const AdaptiveBlockSDFBuildReport& report) {
  const std::filesystem::path path(path_string);
  ensureParent(path);
  std::ofstream file(path);
  file << toJson(report);
}

}  // namespace adasdf

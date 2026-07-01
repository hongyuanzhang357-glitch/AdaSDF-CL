#include "adasdf/generation/AdaptiveBlockSDFBuildReportWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

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
  out << "- v1.6 uses brute-force triangle distance, not BVH acceleration.\n";
  out << "- Blocks store dense SDF values; no low-rank compression is applied.\n";
  out << "- SVD/Tucker compression is not implemented in v1.6.0-alpha.\n";
  out << "- GPU-native compressed adaptive query is planned work.\n\n";

  out << "## Next Steps\n\n";
  out << "- Low-rank block compression is planned for v1.7.0-alpha.\n";
  out << "- Surrogate-guided parameter recommendation is planned for v1.8.0-alpha.\n";
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
  out << "  \"low_rank_compression\": \"planned for v1.7.0-alpha\",\n";
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

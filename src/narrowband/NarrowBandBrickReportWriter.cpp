#include "adasdf/narrowband/NarrowBandBrickReportWriter.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace adasdf {
namespace {

void ensureParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::string jsonEscape(const std::string& text) {
  std::ostringstream out;
  for (char c : text) {
    if (c == '"') {
      out << "\\\"";
    } else if (c == '\\') {
      out << "\\\\";
    } else if (c == '\n') {
      out << "\\n";
    } else {
      out << c;
    }
  }
  return out.str();
}

template <typename T>
std::string mapJson(const std::map<int, T>& values) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto& item : values) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"" << item.first << "\":" << item.second;
  }
  out << "}";
  return out.str();
}

std::string stringMapJson(const std::map<std::string, std::size_t>& values) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto& item : values) {
    if (!first) {
      out << ",";
    }
    first = false;
    out << "\"" << jsonEscape(item.first) << "\":" << item.second;
  }
  out << "}";
  return out.str();
}

std::string warningsJson(const std::vector<std::string>& warnings) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < warnings.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\"" << jsonEscape(warnings[i]) << "\"";
  }
  out << "]";
  return out.str();
}

void markdownMap(
    std::ostringstream& out,
    const std::string& title,
    const std::map<int, std::size_t>& values) {
  out << "\n## " << title << "\n\n";
  out << "| Level | Count |\n|---:|---:|\n";
  for (const auto& item : values) {
    out << "| " << item.first << " | " << item.second << " |\n";
  }
}

}  // namespace

std::string NarrowBandBrickReportWriter::toJson(
    const NarrowBandBrickBuildStats& stats,
    const NarrowBandBrickBuildOptions& options) {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"adasdf.narrowband_brick_build_profile.v1\",\n";
  out << "  \"success\": " << (stats.success ? "true" : "false") << ",\n";
  out << "  \"pipeline\": \"decoupled-sampling-octree-compression-brick\",\n";
  out << "  \"cell_local_sidecar_role\": \"diagnostic-future-residual-only\",\n";
  out << "  \"max_sampling_level\": " << options.max_sampling_level << ",\n";
  out << "  \"sampling_mode\": \"" << options.sampling_mode << "\",\n";
  out << "  \"sampling_refine_zero_crossing\": "
      << (options.sampling_refine_zero_crossing ? "true" : "false") << ",\n";
  out << "  \"sampling_refine_contact_band\": "
      << (options.sampling_refine_contact_band ? "true" : "false") << ",\n";
  out << "  \"sampling_refine_curvature_hint\": "
      << (options.sampling_refine_curvature_hint ? "true" : "false") << ",\n";
  out << "  \"sampling_refine_small_gap_hint\": "
      << (options.sampling_refine_small_gap_hint ? "true" : "false") << ",\n";
  out << "  \"tensor_fill\": \"" << toString(options.tensor_fill) << "\",\n";
  out << "  \"compression_mode\": \"" << toString(options.compression_mode) << "\",\n";
  out << "  \"query_backend\": \"" << options.query_backend << "\",\n";
  out << "  \"contact_exact_min_node_ratio\": "
      << options.contact_exact_min_node_ratio << ",\n";
  out << "  \"contact_exact_stencil\": "
      << options.contact_exact_stencil << ",\n";
  out << "  \"zero_crossing_exact_stencil\": "
      << options.zero_crossing_exact_stencil << ",\n";
  out << "  \"contact_exact_from_zero_crossing_cells\": "
      << (options.contact_exact_from_zero_crossing_cells ? "true" : "false")
      << ",\n";
  out << "  \"sign_protected_fill\": "
      << (options.sign_protected_fill ? "true" : "false") << ",\n";
  out << "  \"fill_sign_check\": "
      << (options.fill_sign_check ? "true" : "false") << ",\n";
  out << "  \"fill_sign_fallback\": \""
      << options.fill_sign_fallback << "\",\n";
  out << "  \"sampling_node_count\": " << stats.sampling_node_count << ",\n";
  out << "  \"sampling_node_count_by_level\": "
      << mapJson(stats.sampling_node_count_by_level) << ",\n";
  out << "  \"exact_sample_estimate_by_level\": "
      << mapJson(stats.exact_sample_estimate_by_level) << ",\n";
  out << "  \"interpolated_sample_estimate_by_level\": "
      << mapJson(stats.interpolated_sample_estimate_by_level) << ",\n";
  out << "  \"contact_band_node_count_by_level\": "
      << mapJson(stats.contact_band_node_count_by_level) << ",\n";
  out << "  \"far_field_node_count_by_level\": "
      << mapJson(stats.far_field_node_count_by_level) << ",\n";
  out << "  \"brick_count\": " << stats.brick_count << ",\n";
  out << "  \"brick_count_by_level\": "
      << mapJson(stats.brick_count_by_level) << ",\n";
  out << "  \"tensor_dim_distribution\": "
      << stringMapJson(stats.tensor_dim_distribution) << ",\n";
  out << "  \"total_tensor_nodes\": " << stats.total_tensor_nodes << ",\n";
  out << "  \"total_exact_source_nodes\": "
      << stats.total_exact_source_nodes << ",\n";
  out << "  \"total_interpolated_fill_nodes\": "
      << stats.total_interpolated_fill_nodes << ",\n";
  out << "  \"estimated_compressed_bytes\": "
      << stats.estimated_compressed_bytes << ",\n";
  out << "  \"estimated_expanded_bytes\": "
      << stats.estimated_expanded_bytes << ",\n";
  out << "  \"estimated_active_expanded_bytes\": "
      << stats.estimated_active_expanded_bytes << ",\n";
  out << "  \"max_single_brick_expanded_bytes\": "
      << stats.max_single_brick_expanded_bytes << ",\n";
  out << "  \"brick_split_recommendation_count\": "
      << stats.brick_split_recommendation_count << ",\n";
  out << "  \"brick_merge_recommendation_count\": "
      << stats.brick_merge_recommendation_count << ",\n";
  out << "  \"compressed_block_count\": "
      << stats.compressed_block_count << ",\n";
  out << "  \"dense_fallback_block_count\": "
      << stats.dense_fallback_block_count << ",\n";
  out << "  \"rank_min\": " << stats.rank_min << ",\n";
  out << "  \"rank_mean\": " << stats.rank_mean << ",\n";
  out << "  \"rank_max\": " << stats.rank_max << ",\n";
  out << "  \"contact_band_sign_flip_count\": "
      << stats.contact_band_sign_flip_count << ",\n";
  out << "  \"contact_band_p95_compression_error\": "
      << stats.contact_band_p95_compression_error << ",\n";
  out << "  \"sign_protected_fill_enabled\": "
      << (stats.sign_protected_fill_enabled ? "true" : "false") << ",\n";
  out << "  \"sign_check_node_count\": "
      << stats.sign_check_node_count << ",\n";
  out << "  \"sign_check_mismatch_count\": "
      << stats.sign_check_mismatch_count << ",\n";
  out << "  \"fill_sign_check_mismatch_count\": "
      << stats.fill_sign_check_mismatch_count << ",\n";
  out << "  \"fill_fallback_exact_node_count\": "
      << stats.fill_fallback_exact_node_count << ",\n";
  out << "  \"zero_crossing_cell_count\": "
      << stats.zero_crossing_cell_count << ",\n";
  out << "  \"zero_crossing_risk_cell_count\": "
      << stats.zero_crossing_risk_cell_count << ",\n";
  out << "  \"protected_zero_crossing_cell_count\": "
      << stats.protected_zero_crossing_cell_count << ",\n";
  out << "  \"mesh_load_time_ms\": " << stats.mesh_load_time_ms << ",\n";
  out << "  \"bvh_build_time_ms\": " << stats.bvh_build_time_ms << ",\n";
  out << "  \"sampling_tree_time_ms\": "
      << stats.sampling_tree_time_ms << ",\n";
  out << "  \"brick_planning_time_ms\": "
      << stats.brick_planning_time_ms << ",\n";
  out << "  \"tensor_generation_time_ms\": "
      << stats.tensor_generation_time_ms << ",\n";
  out << "  \"compression_time_ms\": " << stats.compression_time_ms << ",\n";
  out << "  \"write_time_ms\": " << stats.write_time_ms << ",\n";
  out << "  \"total_time_ms\": " << stats.total_time_ms << ",\n";
  out << "  \"warnings\": " << warningsJson(stats.warnings) << "\n";
  out << "}\n";
  return out.str();
}

std::string NarrowBandBrickReportWriter::toMarkdown(
    const NarrowBandBrickBuildStats& stats,
    const NarrowBandBrickBuildOptions& options) {
  std::ostringstream out;
  out << std::setprecision(12);
  out << "# Narrow-Band Brick SDF Build\n\n";
  out << "| Metric | Value |\n|---|---:|\n";
  out << "| success | " << (stats.success ? "true" : "false") << " |\n";
  out << "| pipeline | decoupled-sampling-octree-compression-brick |\n";
  out << "| cell_local_sidecar_role | diagnostic-future-residual-only |\n";
  out << "| max_sampling_level | " << options.max_sampling_level << " |\n";
  out << "| sampling_mode | " << options.sampling_mode << " |\n";
  out << "| sampling_refine_zero_crossing | "
      << (options.sampling_refine_zero_crossing ? "true" : "false") << " |\n";
  out << "| sampling_refine_contact_band | "
      << (options.sampling_refine_contact_band ? "true" : "false") << " |\n";
  out << "| tensor_fill | " << toString(options.tensor_fill) << " |\n";
  out << "| compression_mode | " << toString(options.compression_mode) << " |\n";
  out << "| query_backend | " << options.query_backend << " |\n";
  out << "| contact_exact_min_node_ratio | "
      << options.contact_exact_min_node_ratio << " |\n";
  out << "| contact_exact_stencil | "
      << options.contact_exact_stencil << " |\n";
  out << "| zero_crossing_exact_stencil | "
      << options.zero_crossing_exact_stencil << " |\n";
  out << "| contact_exact_from_zero_crossing_cells | "
      << (options.contact_exact_from_zero_crossing_cells ? "true" : "false")
      << " |\n";
  out << "| sign_protected_fill | "
      << (options.sign_protected_fill ? "true" : "false") << " |\n";
  out << "| fill_sign_check | "
      << (options.fill_sign_check ? "true" : "false") << " |\n";
  out << "| sampling_node_count | " << stats.sampling_node_count << " |\n";
  out << "| brick_count | " << stats.brick_count << " |\n";
  out << "| total_tensor_nodes | " << stats.total_tensor_nodes << " |\n";
  out << "| total_exact_source_nodes | "
      << stats.total_exact_source_nodes << " |\n";
  out << "| total_interpolated_fill_nodes | "
      << stats.total_interpolated_fill_nodes << " |\n";
  out << "| estimated_expanded_bytes | "
      << stats.estimated_expanded_bytes << " |\n";
  out << "| estimated_active_expanded_bytes | "
      << stats.estimated_active_expanded_bytes << " |\n";
  out << "| brick_split_recommendation_count | "
      << stats.brick_split_recommendation_count << " |\n";
  out << "| brick_merge_recommendation_count | "
      << stats.brick_merge_recommendation_count << " |\n";
  out << "| dense_fallback_block_count | "
      << stats.dense_fallback_block_count << " |\n";
  out << "| compressed_block_count | "
      << stats.compressed_block_count << " |\n";
  out << "| sign_protected_fill_enabled | "
      << (stats.sign_protected_fill_enabled ? "true" : "false")
      << " |\n";
  out << "| sign_check_node_count | " << stats.sign_check_node_count
      << " |\n";
  out << "| sign_check_mismatch_count | "
      << stats.sign_check_mismatch_count << " |\n";
  out << "| fill_sign_check_mismatch_count | "
      << stats.fill_sign_check_mismatch_count << " |\n";
  out << "| fill_fallback_exact_node_count | "
      << stats.fill_fallback_exact_node_count << " |\n";
  out << "| zero_crossing_cell_count | "
      << stats.zero_crossing_cell_count << " |\n";
  out << "| zero_crossing_risk_cell_count | "
      << stats.zero_crossing_risk_cell_count << " |\n";
  out << "| protected_zero_crossing_cell_count | "
      << stats.protected_zero_crossing_cell_count << " |\n";
  out << "| total_time_ms | " << stats.total_time_ms << " |\n";
  out << "| bvh_build_time_ms | " << stats.bvh_build_time_ms << " |\n";
  out << "| sampling_tree_time_ms | " << stats.sampling_tree_time_ms << " |\n";
  out << "| brick_planning_time_ms | " << stats.brick_planning_time_ms << " |\n";
  out << "| tensor_generation_time_ms | " << stats.tensor_generation_time_ms << " |\n";
  out << "| write_time_ms | " << stats.write_time_ms << " |\n";

  markdownMap(out, "Sampling Nodes By Level", stats.sampling_node_count_by_level);
  markdownMap(out, "Contact-Band Nodes By Level", stats.contact_band_node_count_by_level);
  markdownMap(out, "Far-Field Nodes By Level", stats.far_field_node_count_by_level);
  markdownMap(out, "Brick Count By Level", stats.brick_count_by_level);

  out << "\n## Tensor Dimension Distribution\n\n";
  out << "| Tensor dim | Count |\n|---|---:|\n";
  for (const auto& item : stats.tensor_dim_distribution) {
    out << "| " << item.first << " | " << item.second << " |\n";
  }
  out << "\n## Warnings\n\n";
  for (const std::string& warning : stats.warnings) {
    out << "- " << warning << "\n";
  }
  return out.str();
}

bool NarrowBandBrickReportWriter::writeJson(
    const std::filesystem::path& path,
    const NarrowBandBrickBuildStats& stats,
    const NarrowBandBrickBuildOptions& options) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << toJson(stats, options);
  return true;
}

bool NarrowBandBrickReportWriter::writeMarkdown(
    const std::filesystem::path& path,
    const NarrowBandBrickBuildStats& stats,
    const NarrowBandBrickBuildOptions& options) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << toMarkdown(stats, options);
  return true;
}

bool NarrowBandBrickReportWriter::writeProgressJsonl(
    const std::filesystem::path& path,
    const NarrowBandBrickBuildStats& stats) {
  ensureParent(path);
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  file << "{\"stage\":\"complete\",\"success\":"
       << (stats.success ? "true" : "false")
       << ",\"total_time_ms\":" << std::setprecision(17)
       << stats.total_time_ms << "}\n";
  return true;
}

}  // namespace adasdf

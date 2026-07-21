#include "adasdf/generation/AdaptiveTreeStatsWriter.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "adasdf/contract/JsonContractWriter.h"
#include "adasdf/version.h"

namespace adasdf {
namespace {

std::string levelJson(const AdaptiveTreeLevelStats& level) {
  return "{\"level\":" + JsonContractWriter::integer(level.level) +
         ",\"internal_node_count\":" +
         JsonContractWriter::integer(level.internal_node_count) +
         ",\"internal_nodes\":" +
         JsonContractWriter::integer(level.internal_node_count) +
         ",\"refined_node_count\":" +
         JsonContractWriter::integer(level.refined_node_count) +
         ",\"refined_nodes\":" +
         JsonContractWriter::integer(level.refined_node_count) +
         ",\"leaf_block_count\":" +
         JsonContractWriter::integer(level.leaf_block_count) +
         ",\"leaf_blocks\":" +
         JsonContractWriter::integer(level.leaf_block_count) +
         ",\"far_field_leaf_count\":" +
         JsonContractWriter::integer(level.far_field_leaf_count) +
         ",\"far_field\":" +
         JsonContractWriter::integer(level.far_field_leaf_count) +
         ",\"contact_band_leaf_count\":" +
         JsonContractWriter::integer(level.contact_band_leaf_count) +
         ",\"contact_band\":" +
         JsonContractWriter::integer(level.contact_band_leaf_count) +
         ",\"near_surface_leaf_count\":" +
         JsonContractWriter::integer(level.near_surface_leaf_count) +
         ",\"near_surface\":" +
         JsonContractWriter::integer(level.near_surface_leaf_count) +
         ",\"transition_leaf_count\":" +
         JsonContractWriter::integer(level.transition_leaf_count) +
         ",\"transition\":" +
         JsonContractWriter::integer(level.transition_leaf_count) +
         ",\"exact_node_count\":" +
         JsonContractWriter::integer(level.exact_node_count) +
         ",\"exact_nodes\":" +
         JsonContractWriter::integer(level.exact_node_count) +
         ",\"predicted_node_count\":" +
         JsonContractWriter::integer(level.predicted_node_count) +
         ",\"predicted_nodes\":" +
         JsonContractWriter::integer(level.predicted_node_count) +
         ",\"far_field_node_count\":" +
         JsonContractWriter::integer(level.far_field_node_count) +
         ",\"logical_node_count\":" +
         JsonContractWriter::integer(level.logical_node_count) +
         ",\"logical_nodes\":" +
         JsonContractWriter::integer(level.logical_node_count) +
         ",\"block_resolution\":" +
         JsonContractWriter::integer(level.block_resolution) +
         ",\"nodes_per_block\":" +
         JsonContractWriter::integer(level.nodes_per_block) +
         ",\"avg_block_size\":[" +
         JsonContractWriter::number(level.avg_block_size_x) + "," +
         JsonContractWriter::number(level.avg_block_size_y) + "," +
         JsonContractWriter::number(level.avg_block_size_z) + "]" +
         ",\"avg_cell_size\":[" +
         JsonContractWriter::number(level.avg_cell_size_x) + "," +
         JsonContractWriter::number(level.avg_cell_size_y) + "," +
         JsonContractWriter::number(level.avg_cell_size_z) + "]" +
         ",\"block_size\":[" +
         JsonContractWriter::number(level.avg_block_size_x) + "," +
         JsonContractWriter::number(level.avg_block_size_y) + "," +
         JsonContractWriter::number(level.avg_block_size_z) + "]" +
         ",\"cell_size\":[" +
         JsonContractWriter::number(level.avg_cell_size_x) + "," +
         JsonContractWriter::number(level.avg_cell_size_y) + "," +
         JsonContractWriter::number(level.avg_cell_size_z) + "]}";
}

std::string levelsJson(const AdaptiveTreeStats& stats) {
  std::string out = "[";
  for (std::size_t i = 0; i < stats.levels.size(); ++i) {
    if (i > 0) {
      out += ",";
    }
    out += levelJson(stats.levels[i]);
  }
  out += "]";
  return out;
}

std::string boundsJson(const AABB& bounds) {
  return "{\"valid\":" + JsonContractWriter::boolean(bounds.valid) +
         ",\"min\":[" + JsonContractWriter::number(bounds.min.x) + "," +
         JsonContractWriter::number(bounds.min.y) + "," +
         JsonContractWriter::number(bounds.min.z) + "]" +
         ",\"max\":[" + JsonContractWriter::number(bounds.max.x) + "," +
         JsonContractWriter::number(bounds.max.y) + "," +
         JsonContractWriter::number(bounds.max.z) + "]}";
}

std::string warningsJson(const std::vector<std::string>& warnings) {
  std::string out = "[";
  for (std::size_t i = 0; i < warnings.size(); ++i) {
    if (i > 0) {
      out += ",";
    }
    out += JsonContractWriter::quote(warnings[i]);
  }
  out += "]";
  return out;
}

}  // namespace

std::string AdaptiveTreeStatsWriter::toJson(
    const AdaptiveTreeStats& stats,
    const std::string& tool_name) {
  return "{\"schema_id\":\"adasdf.adaptive_tree_stats.v1\"" +
         std::string(",\"schema_version\":1") +
         std::string(",\"tool_name\":") +
         JsonContractWriter::quote(tool_name) +
         ",\"adasdf_version\":" +
         JsonContractWriter::quote(versionString()) +
         ",\"min_level\":" + JsonContractWriter::integer(stats.min_level) +
         ",\"max_level\":" + JsonContractWriter::integer(stats.max_level) +
         ",\"max_level_used\":" +
         JsonContractWriter::integer(stats.max_level_used) +
         ",\"block_resolution\":" +
         JsonContractWriter::integer(stats.block_resolution) +
         ",\"domain_bounds\":" + boundsJson(stats.domain_bounds) +
         ",\"octree_node_count\":" +
         JsonContractWriter::integer(stats.octree_node_count) +
         ",\"internal_node_count\":" +
         JsonContractWriter::integer(stats.internal_node_count) +
         ",\"total_internal_node_count\":" +
         JsonContractWriter::integer(stats.internal_node_count) +
         ",\"refined_node_count\":" +
         JsonContractWriter::integer(stats.refined_node_count) +
         ",\"total_refined_node_count\":" +
         JsonContractWriter::integer(stats.refined_node_count) +
         ",\"leaf_block_count\":" +
         JsonContractWriter::integer(stats.leaf_block_count) +
         ",\"total_leaf_block_count\":" +
         JsonContractWriter::integer(stats.leaf_block_count) +
         ",\"near_surface_leaf_block_count\":" +
         JsonContractWriter::integer(stats.near_surface_leaf_block_count) +
         ",\"exact_node_count\":" +
         JsonContractWriter::integer(stats.exact_node_count) +
         ",\"total_exact_node_count\":" +
         JsonContractWriter::integer(stats.exact_node_count) +
         ",\"predicted_node_count\":" +
         JsonContractWriter::integer(stats.predicted_node_count) +
         ",\"total_predicted_node_count\":" +
         JsonContractWriter::integer(stats.predicted_node_count) +
         ",\"far_field_node_count\":" +
         JsonContractWriter::integer(stats.far_field_node_count) +
         ",\"total_logical_node_count\":" +
         JsonContractWriter::integer(stats.total_logical_node_count) +
         ",\"uniform_max_level_leaf_count\":" +
         JsonContractWriter::integer(stats.uniform_max_level_leaf_count) +
         ",\"theoretical_uniform_leaf_blocks_at_max_level\":" +
         JsonContractWriter::integer(stats.uniform_max_level_leaf_count) +
         ",\"uniform_max_level_logical_node_count\":" +
         JsonContractWriter::integer(
             stats.uniform_max_level_logical_node_count) +
         ",\"theoretical_uniform_logical_nodes_at_max_level\":" +
         JsonContractWriter::integer(
             stats.uniform_max_level_logical_node_count) +
         ",\"sparsity_ratio_vs_uniform_max_level\":" +
         JsonContractWriter::number(
             stats.sparsity_ratio_vs_uniform_max_level) +
         ",\"leaf_sparsity_ratio\":" +
         JsonContractWriter::number(
             stats.uniform_max_level_leaf_count > 0
                 ? static_cast<double>(stats.leaf_block_count) /
                       static_cast<double>(
                           stats.uniform_max_level_leaf_count)
                 : 0.0) +
         ",\"logical_node_sparsity_ratio\":" +
         JsonContractWriter::number(
             stats.sparsity_ratio_vs_uniform_max_level) +
         ",\"appears_uniform_max_level\":" +
         JsonContractWriter::boolean(stats.appears_uniform_max_level) +
         ",\"mixed_level_present\":" +
         JsonContractWriter::boolean(stats.mixed_level_present) +
         ",\"mixed_level_leaves_present\":" +
         JsonContractWriter::boolean(stats.mixed_level_present) +
         ",\"warnings\":" + warningsJson(stats.warnings) +
         ",\"levels\":" + levelsJson(stats) + "}";
}

std::string AdaptiveTreeStatsWriter::toMarkdown(
    const AdaptiveTreeStats& stats) {
  std::ostringstream out;
  out << "# AdaSDF-CL Adaptive Tree Diagnostics\n\n";
  out << "- Leaf blocks: " << stats.leaf_block_count << "\n";
  out << "- Octree nodes: " << stats.octree_node_count << "\n";
  out << "- Max level used: " << stats.max_level_used << "\n";
  out << "- Total logical nodes: " << stats.total_logical_node_count << "\n";
  out << "- Uniform max-level logical nodes: "
      << stats.uniform_max_level_logical_node_count << "\n";
  out << "- Sparsity ratio vs uniform max-level: "
      << stats.sparsity_ratio_vs_uniform_max_level << "\n";
  out << "- Appears uniform max-level: "
      << (stats.appears_uniform_max_level ? "yes" : "no") << "\n";
  out << "- Mixed levels present: "
      << (stats.mixed_level_present ? "yes" : "no") << "\n\n";
  out << "| level | leaf_blocks | internal_nodes | refined_nodes | far_field | "
         "contact_band | near_surface | block_size | cell_size | "
         "nodes_per_block | logical_nodes | exact_nodes | predicted_nodes |\n";
  out << "|---:|---:|---:|---:|---:|---:|---:|---|---|---:|---:|---:|---:|\n";
  for (const AdaptiveTreeLevelStats& level : stats.levels) {
    out << "|" << level.level << "|" << level.leaf_block_count << "|"
        << level.internal_node_count << "|" << level.refined_node_count
        << "|" << level.far_field_leaf_count << "|"
        << level.contact_band_leaf_count << "|"
        << level.near_surface_leaf_count << "|"
        << level.avg_block_size_x << ", " << level.avg_block_size_y << ", "
        << level.avg_block_size_z << "|" << level.avg_cell_size_x << ", "
        << level.avg_cell_size_y << ", " << level.avg_cell_size_z << "|"
        << level.nodes_per_block << "|" << level.logical_node_count << "|"
        << level.exact_node_count << "|" << level.predicted_node_count
        << "|\n";
  }
  return out.str();
}

bool AdaptiveTreeStatsWriter::writeJson(
    const std::string& path,
    const AdaptiveTreeStats& stats,
    const std::string& tool_name) {
  if (path.empty()) {
    return true;
  }
  const std::filesystem::path output(path);
  if (!output.parent_path().empty()) {
    std::filesystem::create_directories(output.parent_path());
  }
  std::ofstream file(output);
  if (!file) {
    return false;
  }
  file << toJson(stats, tool_name);
  return static_cast<bool>(file);
}

bool AdaptiveTreeStatsWriter::writeMarkdown(
    const std::string& path,
    const AdaptiveTreeStats& stats) {
  if (path.empty()) {
    return true;
  }
  const std::filesystem::path output(path);
  if (!output.parent_path().empty()) {
    std::filesystem::create_directories(output.parent_path());
  }
  std::ofstream file(output);
  if (!file) {
    return false;
  }
  file << toMarkdown(stats);
  return static_cast<bool>(file);
}

}  // namespace adasdf

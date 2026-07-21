#include "adasdf/profile/BuildProfileJsonWriter.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "adasdf/contract/BackendJsonContract.h"
#include "adasdf/contract/JsonContractWriter.h"
#include "adasdf/contract/SchemaIds.h"
#include "adasdf/contract/Status.h"

namespace adasdf {

namespace {

std::string timingsJson(const BuildProfileTimings& t) {
  return "{\"load_mesh_time_ms\":" +
         JsonContractWriter::number(t.load_mesh_time_ms) +
         ",\"mesh_preprocess_time_ms\":" +
         JsonContractWriter::number(t.mesh_preprocess_time_ms) +
         ",\"bvh_build_time_ms\":" +
         JsonContractWriter::number(t.bvh_build_time_ms) +
         ",\"distance_query_time_ms\":" +
         JsonContractWriter::number(t.distance_query_time_ms) +
         ",\"sign_query_time_ms\":" +
         JsonContractWriter::number(t.sign_query_time_ms) +
         ",\"adaptive_refinement_time_ms\":" +
         JsonContractWriter::number(t.adaptive_refinement_time_ms) +
         ",\"contact_band_marker_time_ms\":" +
         JsonContractWriter::number(t.contact_band_marker_time_ms) +
         ",\"compression_time_ms\":" +
         JsonContractWriter::number(t.compression_time_ms) +
         ",\"write_model_time_ms\":" +
         JsonContractWriter::number(t.write_model_time_ms) +
         ",\"metadata_time_ms\":" +
         JsonContractWriter::number(t.metadata_time_ms) +
         ",\"cache_lookup_time_ms\":" +
         JsonContractWriter::number(t.cache_lookup_time_ms) +
         ",\"cache_insert_time_ms\":" +
         JsonContractWriter::number(t.cache_insert_time_ms) +
         ",\"deduplication_time_ms\":" +
         JsonContractWriter::number(t.deduplication_time_ms) +
         ",\"marker_cache_time_ms\":" +
         JsonContractWriter::number(t.marker_cache_time_ms) +
         ",\"total_time_ms\":" +
         JsonContractWriter::number(t.total_time_ms) + "}";
}

std::string levelCountArrayJson(
    const std::vector<AdaptiveTreeLevelStats>& levels,
    std::size_t AdaptiveTreeLevelStats::*field) {
  std::string json = "[";
  for (std::size_t i = 0; i < levels.size(); ++i) {
    if (i > 0) {
      json += ",";
    }
    json += "{\"level\":" + JsonContractWriter::integer(levels[i].level) +
            ",\"count\":" + JsonContractWriter::integer(levels[i].*field) +
            "}";
  }
  json += "]";
  return json;
}

std::string vector3Json(double x, double y, double z) {
  return "[" + JsonContractWriter::number(x) + "," +
         JsonContractWriter::number(y) + "," +
         JsonContractWriter::number(z) + "]";
}

std::string adaptiveTreeLevelsJson(
    const std::vector<AdaptiveTreeLevelStats>& levels) {
  std::string json = "[";
  for (std::size_t i = 0; i < levels.size(); ++i) {
    const AdaptiveTreeLevelStats& level = levels[i];
    if (i > 0) {
      json += ",";
    }
    json += "{\"level\":" + JsonContractWriter::integer(level.level) +
            ",\"leaf_blocks\":" +
            JsonContractWriter::integer(level.leaf_block_count) +
            ",\"internal_nodes\":" +
            JsonContractWriter::integer(level.internal_node_count) +
            ",\"refined_nodes\":" +
            JsonContractWriter::integer(level.refined_node_count) +
            ",\"far_field\":" +
            JsonContractWriter::integer(level.far_field_leaf_count) +
            ",\"contact_band\":" +
            JsonContractWriter::integer(level.contact_band_leaf_count) +
            ",\"near_surface\":" +
            JsonContractWriter::integer(level.near_surface_leaf_count) +
            ",\"logical_nodes\":" +
            JsonContractWriter::integer(level.logical_node_count) +
            ",\"exact_nodes\":" +
            JsonContractWriter::integer(level.exact_node_count) +
            ",\"predicted_nodes\":" +
            JsonContractWriter::integer(level.predicted_node_count) +
            ",\"block_size\":" +
            vector3Json(
                level.avg_block_size_x,
                level.avg_block_size_y,
                level.avg_block_size_z) +
            ",\"cell_size\":" +
            vector3Json(
                level.avg_cell_size_x,
                level.avg_cell_size_y,
                level.avg_cell_size_z) +
            "}";
  }
  json += "]";
  return json;
}

std::string adaptiveTreeJson(const AdaptiveTreeStats& stats) {
  return "{\"min_level\":" + JsonContractWriter::integer(stats.min_level) +
         ",\"max_level\":" + JsonContractWriter::integer(stats.max_level) +
         ",\"block_resolution\":" +
         JsonContractWriter::integer(stats.block_resolution) +
         ",\"leaf_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::leaf_block_count) +
         ",\"internal_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::internal_node_count) +
         ",\"refined_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::refined_node_count) +
         ",\"far_field_leaf_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::far_field_leaf_count) +
         ",\"contact_band_leaf_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::contact_band_leaf_count) +
         ",\"near_surface_leaf_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::near_surface_leaf_count) +
         ",\"logical_node_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::logical_node_count) +
         ",\"exact_node_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::exact_node_count) +
         ",\"predicted_node_count_by_level\":" +
         levelCountArrayJson(
             stats.levels,
             &AdaptiveTreeLevelStats::predicted_node_count) +
         ",\"theoretical_uniform_leaf_blocks_at_max_level\":" +
         JsonContractWriter::integer(stats.uniform_max_level_leaf_count) +
         ",\"theoretical_uniform_logical_nodes_at_max_level\":" +
         JsonContractWriter::integer(
             stats.uniform_max_level_logical_node_count) +
         ",\"leaf_sparsity_ratio\":" +
         JsonContractWriter::number(stats.sparsity_ratio_vs_uniform_max_level) +
         ",\"logical_node_sparsity_ratio\":" +
         JsonContractWriter::number(stats.sparsity_ratio_vs_uniform_max_level) +
         ",\"appears_uniform_max_level\":" +
         JsonContractWriter::boolean(stats.appears_uniform_max_level) +
         ",\"mixed_level_leaves_present\":" +
         JsonContractWriter::boolean(stats.mixed_level_present) +
         ",\"levels\":" + adaptiveTreeLevelsJson(stats.levels) + "}";
}

std::string countersJson(const BuildProfileCounters& c) {
  return "{\"num_vertices\":" + JsonContractWriter::integer(c.num_vertices) +
         ",\"num_triangles\":" +
         JsonContractWriter::integer(c.num_triangles) +
         ",\"num_grid_points\":" +
         JsonContractWriter::integer(c.num_grid_points) +
         ",\"num_blocks\":" + JsonContractWriter::integer(c.num_blocks) +
         ",\"num_adaptive_tree_nodes\":" +
         JsonContractWriter::integer(c.num_adaptive_tree_nodes) +
         ",\"num_adaptive_leaf_blocks\":" +
         JsonContractWriter::integer(c.num_adaptive_leaf_blocks) +
         ",\"uniform_max_level_leaf_count\":" +
         JsonContractWriter::integer(c.uniform_max_level_leaf_count) +
         ",\"total_logical_node_count\":" +
         JsonContractWriter::integer(c.total_logical_node_count) +
         ",\"uniform_max_level_logical_node_count\":" +
         JsonContractWriter::integer(
             c.uniform_max_level_logical_node_count) +
         ",\"adaptive_tree_sparsity_ratio\":" +
         JsonContractWriter::number(c.adaptive_tree_sparsity_ratio) +
         ",\"num_contact_band_blocks\":" +
         JsonContractWriter::integer(c.num_contact_band_blocks) +
         ",\"num_distance_queries\":" +
         JsonContractWriter::integer(c.num_distance_queries) +
         ",\"num_sign_queries\":" +
         JsonContractWriter::integer(c.num_sign_queries) +
         ",\"num_triangle_tests_total\":" +
         JsonContractWriter::integer(c.num_triangle_tests_total) +
         ",\"avg_triangles_tested_per_query\":" +
         JsonContractWriter::number(c.avg_triangles_tested_per_query) +
         ",\"bvh_node_visit_count\":" +
         JsonContractWriter::integer(c.bvh_node_visit_count) +
         ",\"exact_node_count\":" +
         JsonContractWriter::integer(c.exact_node_count) +
         ",\"predicted_node_count\":" +
         JsonContractWriter::integer(c.predicted_node_count) +
         ",\"fallback_count\":" +
         JsonContractWriter::integer(c.fallback_count) +
         ",\"compressed_block_count\":" +
         JsonContractWriter::integer(c.compressed_block_count) +
         ",\"dense_fallback_block_count\":" +
         JsonContractWriter::integer(c.dense_fallback_block_count) +
         ",\"output_bytes\":" + JsonContractWriter::integer(c.output_bytes) +
         ",\"compression_guard_enabled\":" +
         JsonContractWriter::boolean(c.compression_guard_enabled) +
         ",\"guarded_block_count\":" +
         JsonContractWriter::integer(c.guarded_block_count) +
         ",\"kept_dense_due_to_sign_count\":" +
         JsonContractWriter::integer(c.kept_dense_due_to_sign_count) +
         ",\"kept_dense_due_to_error_count\":" +
         JsonContractWriter::integer(c.kept_dense_due_to_error_count) +
         ",\"near_zero_compression_sign_flip_count\":" +
         JsonContractWriter::integer(
             c.near_zero_compression_sign_flip_count) +
         ",\"near_zero_compression_p95_error\":" +
         JsonContractWriter::number(c.near_zero_compression_p95_error) +
         ",\"dense_fallback_memory_bytes\":" +
         JsonContractWriter::integer(c.dense_fallback_memory_bytes) +
         ",\"compressed_memory_bytes_after_guard\":" +
         JsonContractWriter::integer(
             c.compressed_memory_bytes_after_guard) +
         ",\"sample_cache_enabled\":" +
         JsonContractWriter::boolean(c.sample_cache_enabled) +
         ",\"sample_cache_scope\":" +
         JsonContractWriter::quote(c.sample_cache_scope) +
         ",\"sample_cache_entries\":" +
         JsonContractWriter::integer(c.sample_cache_entries) +
         ",\"sample_cache_hits\":" +
         JsonContractWriter::integer(c.sample_cache_hits) +
         ",\"sample_cache_misses\":" +
         JsonContractWriter::integer(c.sample_cache_misses) +
         ",\"sample_cache_hit_rate\":" +
         JsonContractWriter::number(c.sample_cache_hit_rate) +
         ",\"distance_cache_hits\":" +
         JsonContractWriter::integer(c.distance_cache_hits) +
         ",\"distance_cache_misses\":" +
         JsonContractWriter::integer(c.distance_cache_misses) +
         ",\"sign_cache_hits\":" +
         JsonContractWriter::integer(c.sign_cache_hits) +
         ",\"sign_cache_misses\":" +
         JsonContractWriter::integer(c.sign_cache_misses) +
         ",\"corner_cache_hits\":" +
         JsonContractWriter::integer(c.corner_cache_hits) +
         ",\"corner_cache_misses\":" +
         JsonContractWriter::integer(c.corner_cache_misses) +
         ",\"block_point_duplicate_count\":" +
         JsonContractWriter::integer(c.block_point_duplicate_count) +
         ",\"marker_candidate_cache_hits\":" +
         JsonContractWriter::integer(c.marker_candidate_cache_hits) +
         ",\"marker_candidate_cache_misses\":" +
         JsonContractWriter::integer(c.marker_candidate_cache_misses) +
         ",\"marker_decision_cache_hits\":" +
         JsonContractWriter::integer(c.marker_decision_cache_hits) +
         ",\"marker_decision_cache_misses\":" +
         JsonContractWriter::integer(c.marker_decision_cache_misses) +
         ",\"distance_queries_saved\":" +
         JsonContractWriter::integer(c.distance_queries_saved) +
         ",\"sign_queries_saved\":" +
         JsonContractWriter::integer(c.sign_queries_saved) +
         ",\"box_triangle_distance_saved\":" +
         JsonContractWriter::integer(c.box_triangle_distance_saved) +
         ",\"cache_memory_estimate_bytes\":" +
         JsonContractWriter::integer(c.cache_memory_estimate_bytes) +
         "}";
}

}  // namespace

std::string BuildProfileJsonWriter::toJson(const BuildProfiler& profiler) {
  BackendJsonContract contract;
  contract.schema_id = SchemaIds::BuildProfile;
  contract.tool_name = profiler.tool_name;
  contract.status = profiler.success ? JsonStatus::Ok : JsonStatus::Error;
  contract.status_code = profiler.status_code;
  contract.success = profiler.success;
  contract.warnings = profiler.warnings;
  contract.payload_fields.push_back(
      {"profile_status", JsonContractWriter::quote(profiler.status)});
  contract.payload_fields.push_back(
      {"input_path", JsonContractWriter::quote(profiler.input_path)});
  contract.payload_fields.push_back(
      {"output_path", JsonContractWriter::quote(profiler.output_path)});
  contract.payload_fields.push_back({"timings", timingsJson(profiler.timings)});
  contract.payload_fields.push_back({"counters", countersJson(profiler.counters)});
  if (profiler.counters.has_adaptive_tree_stats) {
    contract.payload_fields.push_back(
        {"adaptive_tree", adaptiveTreeJson(profiler.counters.adaptive_tree)});
  }
  contract.payload_fields.push_back(
      {"error_message", JsonContractWriter::quote(profiler.error_message)});
  return JsonContractWriter::writeObject(contract);
}

bool BuildProfileJsonWriter::write(
    const std::string& path,
    const BuildProfiler& profiler) {
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
  file << toJson(profiler);
  return static_cast<bool>(file);
}

}  // namespace adasdf

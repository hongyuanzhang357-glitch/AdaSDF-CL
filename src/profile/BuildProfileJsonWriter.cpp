#include "adasdf/profile/BuildProfileJsonWriter.h"

#include <filesystem>
#include <fstream>

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
         ",\"total_time_ms\":" +
         JsonContractWriter::number(t.total_time_ms) + "}";
}

std::string countersJson(const BuildProfileCounters& c) {
  return "{\"num_vertices\":" + JsonContractWriter::integer(c.num_vertices) +
         ",\"num_triangles\":" +
         JsonContractWriter::integer(c.num_triangles) +
         ",\"num_grid_points\":" +
         JsonContractWriter::integer(c.num_grid_points) +
         ",\"num_blocks\":" + JsonContractWriter::integer(c.num_blocks) +
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

enum class SDFCreationStatus {
  Feasible,
  Infeasible,
  InvalidInput,
  BuildFailed,
};

enum class SDFHardConstraint {
  MaxSDFFileBytes,
  MaxDecodedBlockBytes,
  MaxZeroSurfaceAbsError,
};

// This is the complete public tuning contract for constrained SDF creation.
// Backend parameters are selected internally and are only exposed in reports.
struct SDFCreationConstraints {
  std::uint64_t max_sdf_file_bytes = 0;
  std::uint64_t max_decoded_block_bytes = 0;
  double max_zero_surface_abs_error = 0.0;
};

struct SDFCreationMeasurements {
  std::uint64_t sdf_file_bytes = 0;
  std::uint64_t max_decoded_block_bytes = 0;
  double max_zero_surface_abs_error = 0.0;
  bool serialized_round_trip_verified = false;
  bool zero_surface_error_is_strict_bound = false;
  bool zero_surface_validation_complete = false;
  std::uint64_t zero_surface_validation_sample_count = 0;
  double zero_surface_validation_max_spacing = 0.0;
  double mesh_to_sdf_max_abs_phi = 0.0;
  double sdf_to_mesh_max_abs_distance = 0.0;
  std::uint64_t zero_crossing_validation_sample_count = 0;
};

struct BackendBuildParameters {
  std::uint32_t deterministic_seed = 1337;
  int thread_count = 1;
  int bvh_leaf_size = 8;
  int octree_min_depth = 1;
  int octree_max_depth = 5;
  double octree_narrow_band_scale = 1.5;
  double octree_interpolation_residual_scale = 0.25;
  int octree_max_overlapping_triangles = 16;
  double octree_max_normal_complexity = 0.15;
  int block_min_nodes = 8;
  int block_target_nodes = 32;
  int block_max_nodes = 32768;
  int block_halo = 1;
  int block_min_tensor_dimension = 3;
  int block_max_tensor_dimension = 33;
  std::string compression_method = "auto";
  double compression_abs_tolerance = 0.0;
  int compression_max_rank = 0;
};

using SDFBackendParameters = BackendBuildParameters;

struct SDFCreationAttempt {
  int attempt_index = 0;
  SDFBackendParameters parameters;
  SDFCreationMeasurements measurements;
  std::vector<SDFHardConstraint> failed_constraints;
  double bvh_build_time_ms = 0.0;
  double octree_build_time_ms = 0.0;
  double block_partition_time_ms = 0.0;
  double tensor_build_time_ms = 0.0;
  double exact_phi_time_ms = 0.0;
  double interpolation_time_ms = 0.0;
  double compression_time_ms = 0.0;
  double serialization_time_ms = 0.0;
  double reload_validation_time_ms = 0.0;
  double zero_surface_validation_time_ms = 0.0;
  double validation_time_ms = 0.0;
  double query_time_ms = 0.0;
  std::uint64_t query_sample_count = 0;
  double query_ns_per_sample = 0.0;
  double total_time_ms = 0.0;
  std::size_t exact_node_count = 0;
  std::size_t exact_query_requests = 0;
  std::size_t unique_exact_queries = 0;
  std::size_t exact_query_cache_hits = 0;
  std::size_t interpolated_node_count = 0;
  std::size_t promoted_node_count = 0;
  std::size_t bvh_node_visits = 0;
  std::size_t triangle_tests = 0;
  std::size_t octree_node_count = 0;
  std::size_t octree_leaf_count = 0;
  std::size_t uniform_grid_node_count = 0;
  std::size_t block_count = 0;
  std::size_t compressed_block_count = 0;
  std::size_t dense_fallback_block_count = 0;
  std::uint64_t original_tensor_bytes = 0;
  std::uint64_t compressed_tensor_bytes = 0;
  int rank_min = 0;
  int rank_max = 0;
  std::string message;
};

struct SDFCreationSuggestions {
  std::uint64_t max_sdf_file_bytes = 0;
  std::uint64_t max_decoded_block_bytes = 0;
  double max_zero_surface_abs_error = 0.0;
};

struct SDFCreationReport {
  SDFCreationStatus status = SDFCreationStatus::BuildFailed;
  std::string library_version;
  std::string mesh_hash;
  std::string feature_schema;
  std::string advisor_source;
  bool advisor_used_fallback = false;
  bool advisor_out_of_distribution = false;
  std::vector<std::string> advisor_warnings;
  std::map<std::string, double> geometry_features;
  SDFCreationConstraints requested;
  SDFCreationMeasurements actual;
  std::vector<SDFHardConstraint> failed_constraints;
  std::vector<SDFCreationAttempt> attempts;
  std::size_t total_exact_query_requests = 0;
  std::size_t total_unique_exact_queries = 0;
  std::size_t total_exact_query_cache_hits = 0;
  std::size_t total_bvh_node_visits = 0;
  std::size_t total_triangle_tests = 0;
  int selected_attempt_index = -1;
  bool has_selected_backend_parameters = false;
  BackendBuildParameters selected_backend_parameters;
  double mesh_feature_time_ms = 0.0;
  double advisor_time_ms = 0.0;
  double search_time_ms = 0.0;
  std::string candidate_cost_order =
      "sdf_file_bytes>total_time_ms>query_time_ms>attempt_index";
  SDFCreationSuggestions suggested_budget;
  std::string message;

  bool feasible() const { return status == SDFCreationStatus::Feasible; }
};

struct SDFCreationResult {
  std::shared_ptr<SDFModel> model;
  SDFCreationReport report;

  bool feasible() const { return model != nullptr && report.feasible(); }
};

const char* toString(SDFCreationStatus status);
const char* toString(SDFHardConstraint constraint);

bool validateSDFCreationConstraints(
    const SDFCreationConstraints& constraints,
    std::string* error_message = nullptr);

bool validateBackendBuildParameters(
    const BackendBuildParameters& parameters,
    std::string* error_message = nullptr);

SDFCreationReport evaluateSDFCreationConstraints(
    const SDFCreationConstraints& constraints,
    const SDFCreationMeasurements& measurements,
    std::vector<SDFCreationAttempt> attempts = {});

std::string toJson(const SDFCreationReport& report);
std::string sdfCreationCsvHeader();
std::string toCsvRow(const SDFCreationReport& report);

}  // namespace adasdf

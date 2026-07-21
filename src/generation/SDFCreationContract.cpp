#include "adasdf/generation/SDFCreationContract.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace adasdf {
namespace {

std::string escapeJson(const std::string& value) {
  std::ostringstream out;
  for (const unsigned char ch : value) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (ch < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(ch) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  return out.str();
}

void writeJsonNumber(std::ostream& out, double value) {
  if (!std::isfinite(value)) {
    out << "null";
    return;
  }
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << value;
}

void writeConstraints(std::ostream& out, const SDFCreationConstraints& value) {
  out << "{\"max_sdf_file_bytes\":" << value.max_sdf_file_bytes
      << ",\"max_decoded_block_bytes\":"
      << value.max_decoded_block_bytes
      << ",\"max_zero_surface_abs_error\":"
      << std::setprecision(std::numeric_limits<double>::max_digits10)
      << value.max_zero_surface_abs_error << "}";
}

void writeMeasurements(
    std::ostream& out,
    const SDFCreationMeasurements& value) {
  out << "{\"sdf_file_bytes\":" << value.sdf_file_bytes
      << ",\"max_decoded_block_bytes\":"
      << value.max_decoded_block_bytes
      << ",\"max_zero_surface_abs_error\":";
  writeJsonNumber(out, value.max_zero_surface_abs_error);
  out << ",\"serialized_round_trip_verified\":"
      << (value.serialized_round_trip_verified ? "true" : "false")
      << ",\"zero_surface_error_is_strict_bound\":"
      << (value.zero_surface_error_is_strict_bound ? "true" : "false")
      << ",\"zero_surface_validation_complete\":"
      << (value.zero_surface_validation_complete ? "true" : "false")
      << ",\"zero_surface_validation_sample_count\":"
      << value.zero_surface_validation_sample_count
      << ",\"zero_surface_validation_max_spacing\":";
  writeJsonNumber(out, value.zero_surface_validation_max_spacing);
  out << ",\"mesh_to_sdf_max_abs_phi\":";
  writeJsonNumber(out, value.mesh_to_sdf_max_abs_phi);
  out << ",\"sdf_to_mesh_max_abs_distance\":";
  writeJsonNumber(out, value.sdf_to_mesh_max_abs_distance);
  out << ",\"zero_crossing_validation_sample_count\":"
      << value.zero_crossing_validation_sample_count
      << "}";
}

void writeFailures(
    std::ostream& out,
    const std::vector<SDFHardConstraint>& failures) {
  out << "[";
  for (std::size_t i = 0; i < failures.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\"" << toString(failures[i]) << "\"";
  }
  out << "]";
}

void writeParameters(std::ostream& out, const SDFBackendParameters& value) {
  out << "{\"deterministic_seed\":" << value.deterministic_seed
      << ",\"thread_count\":" << value.thread_count
      << ",\"bvh_leaf_size\":" << value.bvh_leaf_size
      << ",\"octree_min_depth\":" << value.octree_min_depth
      << ",\"octree_max_depth\":" << value.octree_max_depth
      << ",\"octree_narrow_band_scale\":"
      << value.octree_narrow_band_scale
      << ",\"octree_interpolation_residual_scale\":"
      << value.octree_interpolation_residual_scale
      << ",\"octree_max_overlapping_triangles\":"
      << value.octree_max_overlapping_triangles
      << ",\"octree_max_normal_complexity\":"
      << value.octree_max_normal_complexity
      << ",\"block_min_nodes\":" << value.block_min_nodes
      << ",\"block_target_nodes\":" << value.block_target_nodes
      << ",\"block_max_nodes\":" << value.block_max_nodes
      << ",\"block_halo\":" << value.block_halo
      << ",\"block_min_tensor_dimension\":"
      << value.block_min_tensor_dimension
      << ",\"block_max_tensor_dimension\":"
      << value.block_max_tensor_dimension
      << ",\"compression_method\":\""
      << escapeJson(value.compression_method) << "\""
      << ",\"compression_abs_tolerance\":"
      << std::setprecision(std::numeric_limits<double>::max_digits10)
      << value.compression_abs_tolerance
      << ",\"compression_max_rank\":" << value.compression_max_rank << "}";
}

}  // namespace

const char* toString(SDFCreationStatus status) {
  switch (status) {
    case SDFCreationStatus::Feasible:
      return "Feasible";
    case SDFCreationStatus::Infeasible:
      return "Infeasible";
    case SDFCreationStatus::InvalidInput:
      return "InvalidInput";
    case SDFCreationStatus::BuildFailed:
      return "BuildFailed";
  }
  return "BuildFailed";
}

const char* toString(SDFHardConstraint constraint) {
  switch (constraint) {
    case SDFHardConstraint::MaxSDFFileBytes:
      return "max_sdf_file_bytes";
    case SDFHardConstraint::MaxDecodedBlockBytes:
      return "max_decoded_block_bytes";
    case SDFHardConstraint::MaxZeroSurfaceAbsError:
      return "max_zero_surface_abs_error";
  }
  return "unknown";
}

bool validateSDFCreationConstraints(
    const SDFCreationConstraints& constraints,
    std::string* error_message) {
  std::string error;
  if (constraints.max_sdf_file_bytes == 0) {
    error = "max_sdf_file_bytes must be greater than zero";
  } else if (constraints.max_decoded_block_bytes == 0) {
    error = "max_decoded_block_bytes must be greater than zero";
  } else if (!(constraints.max_zero_surface_abs_error > 0.0) ||
             !std::isfinite(constraints.max_zero_surface_abs_error)) {
    error = "max_zero_surface_abs_error must be finite and greater than zero";
  }
  if (error_message != nullptr) {
    *error_message = error;
  }
  return error.empty();
}

bool validateBackendBuildParameters(
    const BackendBuildParameters& parameters,
    std::string* error_message) {
  std::string error;
  const bool known_method =
      parameters.compression_method == "Dense" ||
      parameters.compression_method == "MatrixSVD" ||
      parameters.compression_method == "HOSVD" ||
      parameters.compression_method == "TT" ||
      parameters.compression_method == "Tucker";
  if (parameters.thread_count != 1) {
    error = "constrained SDF creation currently requires thread_count=1";
  } else if (parameters.bvh_leaf_size <= 0) {
    error = "bvh_leaf_size must be positive";
  } else if (parameters.octree_min_depth < 0 ||
             parameters.octree_max_depth < parameters.octree_min_depth ||
             parameters.octree_max_depth >= 20) {
    error = "octree depth range is invalid";
  } else if (!(parameters.octree_narrow_band_scale >= 0.0) ||
             !std::isfinite(parameters.octree_narrow_band_scale) ||
             !(parameters.octree_interpolation_residual_scale >= 0.0) ||
             !std::isfinite(
                 parameters.octree_interpolation_residual_scale)) {
    error = "octree scales must be finite and non-negative";
  } else if (parameters.octree_max_overlapping_triangles <= 0 ||
             !(parameters.octree_max_normal_complexity >= 0.0) ||
             !std::isfinite(parameters.octree_max_normal_complexity)) {
    error = "octree geometry thresholds are invalid";
  } else if (parameters.block_min_nodes <= 0 ||
             parameters.block_target_nodes < parameters.block_min_nodes ||
             parameters.block_max_nodes < parameters.block_target_nodes ||
             parameters.block_halo < 0) {
    error = "block node range or halo is invalid";
  } else if (parameters.block_min_tensor_dimension < 2 ||
             parameters.block_max_tensor_dimension <
                 parameters.block_min_tensor_dimension) {
    error = "block tensor dimension range is invalid";
  } else if (!known_method) {
    error = "compression_method is unsupported";
  } else if (!(parameters.compression_abs_tolerance > 0.0) ||
             !std::isfinite(parameters.compression_abs_tolerance) ||
             parameters.compression_max_rank <= 0) {
    error = "compression tolerance and maximum rank are invalid";
  }
  if (error_message != nullptr) {
    *error_message = error;
  }
  return error.empty();
}

SDFCreationReport evaluateSDFCreationConstraints(
    const SDFCreationConstraints& constraints,
    const SDFCreationMeasurements& measurements,
    std::vector<SDFCreationAttempt> attempts) {
  SDFCreationReport report;
  report.requested = constraints;
  report.actual = measurements;
  report.attempts = std::move(attempts);
  for (const SDFCreationAttempt& attempt : report.attempts) {
    report.total_exact_query_requests += attempt.exact_query_requests;
    report.total_unique_exact_queries += attempt.unique_exact_queries;
    report.total_exact_query_cache_hits += attempt.exact_query_cache_hits;
    report.total_bvh_node_visits += attempt.bvh_node_visits;
    report.total_triangle_tests += attempt.triangle_tests;
  }
  std::string validation_error;
  if (!validateSDFCreationConstraints(constraints, &validation_error)) {
    report.status = SDFCreationStatus::InvalidInput;
    report.message = validation_error;
    return report;
  }

  if (measurements.sdf_file_bytes > constraints.max_sdf_file_bytes) {
    report.failed_constraints.push_back(SDFHardConstraint::MaxSDFFileBytes);
  }
  if (measurements.max_decoded_block_bytes >
      constraints.max_decoded_block_bytes) {
    report.failed_constraints.push_back(
        SDFHardConstraint::MaxDecodedBlockBytes);
  }
  if (!std::isfinite(measurements.max_zero_surface_abs_error) ||
      measurements.max_zero_surface_abs_error >
          constraints.max_zero_surface_abs_error) {
    report.failed_constraints.push_back(
        SDFHardConstraint::MaxZeroSurfaceAbsError);
  }

  report.suggested_budget.max_sdf_file_bytes = std::max(
      constraints.max_sdf_file_bytes,
      measurements.sdf_file_bytes);
  report.suggested_budget.max_decoded_block_bytes = std::max(
      constraints.max_decoded_block_bytes,
      measurements.max_decoded_block_bytes);
  report.suggested_budget.max_zero_surface_abs_error =
      std::isfinite(measurements.max_zero_surface_abs_error)
      ? std::max(
            constraints.max_zero_surface_abs_error,
            measurements.max_zero_surface_abs_error)
      : constraints.max_zero_surface_abs_error;

  if (report.failed_constraints.empty()) {
    if (!measurements.serialized_round_trip_verified ||
        !measurements.zero_surface_validation_complete) {
      report.status = SDFCreationStatus::BuildFailed;
      report.message =
          "constraint measurements require serialized reload and a complete "
          "zero-surface audit";
    } else {
      report.status = SDFCreationStatus::Feasible;
      report.message = "all hard constraints satisfied";
    }
  } else {
    report.status = SDFCreationStatus::Infeasible;
    report.message = "one or more hard constraints were exceeded";
  }
  return report;
}

std::string toJson(const SDFCreationReport& report) {
  std::ostringstream out;
  out << "{\"schema_version\":1,\"status\":\""
      << toString(report.status) << "\",\"library_version\":\""
      << escapeJson(report.library_version) << "\",\"mesh_hash\":\""
      << escapeJson(report.mesh_hash) << "\",\"feature_schema\":\""
      << escapeJson(report.feature_schema) << "\",\"advisor_source\":\""
      << escapeJson(report.advisor_source)
      << "\",\"advisor_used_fallback\":"
      << (report.advisor_used_fallback ? "true" : "false")
      << ",\"advisor_out_of_distribution\":"
      << (report.advisor_out_of_distribution ? "true" : "false")
      << ",\"advisor_warnings\":[";
  for (std::size_t i = 0; i < report.advisor_warnings.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\"" << escapeJson(report.advisor_warnings[i]) << "\"";
  }
  out << "],\"geometry_features\":{";
  std::size_t feature_index = 0;
  for (const auto& feature : report.geometry_features) {
    if (feature_index++ > 0) {
      out << ",";
    }
    out << "\"" << escapeJson(feature.first) << "\":"
        << std::setprecision(std::numeric_limits<double>::max_digits10)
        << feature.second;
  }
  out << "},\"selected_attempt_index\":"
      << report.selected_attempt_index
      << ",\"has_selected_backend_parameters\":"
      << (report.has_selected_backend_parameters ? "true" : "false")
      << ",\"selected_backend_parameters\":";
  writeParameters(out, report.selected_backend_parameters);
  out
      << ",\"pipeline_timing_ms\":{"
      << "\"mesh_features\":" << report.mesh_feature_time_ms
      << ",\"advisor\":" << report.advisor_time_ms
      << ",\"search\":" << report.search_time_ms << "}"
      << ",\"candidate_cost_order\":\""
      << escapeJson(report.candidate_cost_order) << "\""
      << ",\"aggregate_oracle_stats\":{"
      << "\"exact_query_requests\":"
      << report.total_exact_query_requests
      << ",\"unique_exact_queries\":"
      << report.total_unique_exact_queries
      << ",\"cache_hits\":" << report.total_exact_query_cache_hits
      << ",\"bvh_node_visits\":" << report.total_bvh_node_visits
      << ",\"triangle_tests\":" << report.total_triangle_tests << "}"
      << ",\"requested\":";
  writeConstraints(out, report.requested);
  out << ",\"actual\":";
  writeMeasurements(out, report.actual);
  out << ",\"failed_constraints\":";
  writeFailures(out, report.failed_constraints);
  out << ",\"attempts\":[";
  for (std::size_t i = 0; i < report.attempts.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    const SDFCreationAttempt& attempt = report.attempts[i];
    out << "{\"attempt_index\":" << attempt.attempt_index
        << ",\"parameters\":";
    writeParameters(out, attempt.parameters);
    out << ",\"measurements\":";
    writeMeasurements(out, attempt.measurements);
    out << ",\"failed_constraints\":";
    writeFailures(out, attempt.failed_constraints);
    out << ",\"timing_ms\":{"
        << "\"bvh\":" << attempt.bvh_build_time_ms
        << ",\"octree\":" << attempt.octree_build_time_ms
        << ",\"block_partition\":" << attempt.block_partition_time_ms
        << ",\"brick_generation\":"
        << attempt.block_partition_time_ms + attempt.tensor_build_time_ms +
            attempt.compression_time_ms
        << ",\"tensor_build\":" << attempt.tensor_build_time_ms
        << ",\"exact_phi\":" << attempt.exact_phi_time_ms
        << ",\"interpolation\":" << attempt.interpolation_time_ms
        << ",\"compression\":" << attempt.compression_time_ms
        << ",\"serialization\":" << attempt.serialization_time_ms
        << ",\"reload_validation\":"
        << attempt.reload_validation_time_ms
        << ",\"zero_surface_validation\":"
        << attempt.zero_surface_validation_time_ms
        << ",\"validation\":" << attempt.validation_time_ms
        << ",\"query\":" << attempt.query_time_ms
        << ",\"total\":" << attempt.total_time_ms << "}"
        << ",\"query_sample_count\":" << attempt.query_sample_count
        << ",\"query_ns_per_sample\":" << attempt.query_ns_per_sample
        << ",\"exact_node_count\":" << attempt.exact_node_count
        << ",\"exact_query_requests\":" << attempt.exact_query_requests
        << ",\"unique_exact_queries\":" << attempt.unique_exact_queries
        << ",\"exact_query_cache_hits\":"
        << attempt.exact_query_cache_hits
        << ",\"interpolated_node_count\":"
        << attempt.interpolated_node_count
        << ",\"promoted_node_count\":" << attempt.promoted_node_count
        << ",\"bvh_node_visits\":" << attempt.bvh_node_visits
        << ",\"triangle_tests\":" << attempt.triangle_tests
        << ",\"octree_node_count\":" << attempt.octree_node_count
        << ",\"octree_leaf_count\":" << attempt.octree_leaf_count
        << ",\"uniform_grid_node_count\":"
        << attempt.uniform_grid_node_count
        << ",\"block_count\":" << attempt.block_count
        << ",\"compressed_block_count\":"
        << attempt.compressed_block_count
        << ",\"dense_fallback_block_count\":"
        << attempt.dense_fallback_block_count
        << ",\"original_tensor_bytes\":"
        << attempt.original_tensor_bytes
        << ",\"compressed_tensor_bytes\":"
        << attempt.compressed_tensor_bytes
        << ",\"rank_min\":" << attempt.rank_min
        << ",\"rank_max\":" << attempt.rank_max
        << ",\"message\":\"" << escapeJson(attempt.message) << "\"}";
  }
  out << "],\"suggested_budget\":{"
      << "\"max_sdf_file_bytes\":"
      << report.suggested_budget.max_sdf_file_bytes
      << ",\"max_decoded_block_bytes\":"
      << report.suggested_budget.max_decoded_block_bytes
      << ",\"max_zero_surface_abs_error\":"
      << std::setprecision(std::numeric_limits<double>::max_digits10)
      << report.suggested_budget.max_zero_surface_abs_error << "}"
      << ",\"message\":\"" << escapeJson(report.message) << "\"}";
  return out.str();
}

std::string sdfCreationCsvHeader() {
  return "status,library_version,mesh_hash,advisor_source,"
         "advisor_used_fallback,selected_attempt_index,"
         "mesh_feature_time_ms,advisor_time_ms,search_time_ms,"
         "candidate_cost_order,max_sdf_file_bytes_budget,"
         "max_decoded_block_bytes_budget,max_zero_surface_abs_error_budget,"
         "sdf_file_bytes_actual,max_decoded_block_bytes_actual,"
         "max_zero_surface_abs_error_actual,error_is_strict_bound,"
         "zero_surface_validation_complete,"
         "zero_surface_validation_sample_count,"
         "zero_surface_validation_max_spacing,"
         "mesh_to_sdf_max_abs_phi,sdf_to_mesh_max_abs_distance,"
         "zero_crossing_validation_sample_count,"
         "attempt_count,octree_node_count,octree_leaf_count,block_count,"
         "exact_node_count,total_exact_query_requests,"
         "total_unique_exact_queries,total_exact_query_cache_hits,"
         "interpolated_node_count,promoted_node_count,"
         "total_bvh_node_visits,total_triangle_tests,compressed_block_count,"
         "dense_fallback_block_count,rank_min,rank_max,total_time_ms,"
         "bvh_time_ms,octree_time_ms,block_partition_time_ms,"
         "brick_generation_time_ms,tensor_build_time_ms,"
         "exact_phi_time_ms,interpolation_time_ms,compression_time_ms,"
         "serialization_time_ms,reload_validation_time_ms,"
         "zero_surface_validation_time_ms,validation_time_ms,"
         "query_time_ms,query_sample_count,query_ns_per_sample";
}

std::string toCsvRow(const SDFCreationReport& report) {
  const SDFCreationAttempt empty;
  const SDFCreationAttempt* selected = nullptr;
  for (const SDFCreationAttempt& candidate : report.attempts) {
    if (candidate.attempt_index == report.selected_attempt_index) {
      selected = &candidate;
      break;
    }
  }
  const SDFCreationAttempt& attempt = selected != nullptr
      ? *selected
      : (report.attempts.empty() ? empty : report.attempts.back());
  std::ostringstream out;
  out << toString(report.status) << "," << report.library_version << ","
      << report.mesh_hash << "," << report.advisor_source << ","
      << (report.advisor_used_fallback ? "true" : "false") << ","
      << report.selected_attempt_index << ","
      << report.mesh_feature_time_ms << ","
      << report.advisor_time_ms << ","
      << report.search_time_ms << ","
      << report.candidate_cost_order << ","
      << report.requested.max_sdf_file_bytes << ","
      << report.requested.max_decoded_block_bytes << ","
      << std::setprecision(std::numeric_limits<double>::max_digits10)
      << report.requested.max_zero_surface_abs_error << ","
      << report.actual.sdf_file_bytes << ","
      << report.actual.max_decoded_block_bytes << ","
      << report.actual.max_zero_surface_abs_error << ","
      << (report.actual.zero_surface_error_is_strict_bound ? "true" : "false")
      << ","
      << (report.actual.zero_surface_validation_complete ? "true" : "false")
      << "," << report.actual.zero_surface_validation_sample_count << ","
      << report.actual.zero_surface_validation_max_spacing << ","
      << report.actual.mesh_to_sdf_max_abs_phi << ","
      << report.actual.sdf_to_mesh_max_abs_distance << ","
      << report.actual.zero_crossing_validation_sample_count << ","
      << report.attempts.size() << "," << attempt.octree_node_count
      << "," << attempt.octree_leaf_count << "," << attempt.block_count
      << "," << attempt.exact_node_count << ","
      << report.total_exact_query_requests << ","
      << report.total_unique_exact_queries << ","
      << report.total_exact_query_cache_hits << ","
      << attempt.interpolated_node_count << "," << attempt.promoted_node_count
      << "," << report.total_bvh_node_visits << ","
      << report.total_triangle_tests
      << "," << attempt.compressed_block_count << ","
      << attempt.dense_fallback_block_count << "," << attempt.rank_min << ","
      << attempt.rank_max << "," << attempt.total_time_ms << ","
      << attempt.bvh_build_time_ms << ","
      << attempt.octree_build_time_ms << ","
      << attempt.block_partition_time_ms << ","
      << attempt.block_partition_time_ms + attempt.tensor_build_time_ms +
          attempt.compression_time_ms << ","
      << attempt.tensor_build_time_ms << ","
      << attempt.exact_phi_time_ms << ","
      << attempt.interpolation_time_ms << ","
      << attempt.compression_time_ms << ","
      << attempt.serialization_time_ms << ","
      << attempt.reload_validation_time_ms << ","
      << attempt.zero_surface_validation_time_ms << ","
      << attempt.validation_time_ms << ","
      << attempt.query_time_ms << "," << attempt.query_sample_count << ","
      << attempt.query_ns_per_sample;
  return out.str();
}

}  // namespace adasdf

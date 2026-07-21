#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>

int main() {
  static_assert(std::is_aggregate<adasdf::SDFCreationConstraints>::value,
                "public constraints must remain a simple aggregate");

  adasdf::SDFCreationConstraints invalid;
  std::string error;
  if (adasdf::validateSDFCreationConstraints(invalid, &error) || error.empty()) {
    std::cerr << "zero budgets must be rejected\n";
    return 1;
  }

  adasdf::SDFCreationConstraints constraints;
  constraints.max_sdf_file_bytes = 1024;
  constraints.max_decoded_block_bytes = 512;
  constraints.max_zero_surface_abs_error = 0.01;
  if (!adasdf::validateSDFCreationConstraints(constraints, &error)) {
    std::cerr << error << "\n";
    return 1;
  }

  adasdf::BackendBuildParameters parameters;
  parameters.compression_method = "Dense";
  parameters.compression_abs_tolerance = 0.0025;
  parameters.compression_max_rank = 8;
  if (!adasdf::validateBackendBuildParameters(parameters, &error)) {
    std::cerr << error << "\n";
    return 1;
  }
  parameters.thread_count = 2;
  if (adasdf::validateBackendBuildParameters(parameters, &error) ||
      error.find("thread_count=1") == std::string::npos) {
    std::cerr << "unsupported explicit thread count was accepted\n";
    return 1;
  }
  parameters.thread_count = 1;
  parameters.compression_method = "auto";
  if (adasdf::validateBackendBuildParameters(parameters, &error)) {
    std::cerr << "non-explicit compression method was accepted\n";
    return 1;
  }

  adasdf::SDFCreationMeasurements feasible_measurements;
  feasible_measurements.sdf_file_bytes = 900;
  feasible_measurements.max_decoded_block_bytes = 400;
  feasible_measurements.max_zero_surface_abs_error = 0.005;
  feasible_measurements.serialized_round_trip_verified = true;
  feasible_measurements.zero_surface_validation_complete = true;
  feasible_measurements.zero_surface_validation_sample_count = 42;
  feasible_measurements.zero_surface_validation_max_spacing = 0.01;
  feasible_measurements.mesh_to_sdf_max_abs_phi = 0.004;
  feasible_measurements.sdf_to_mesh_max_abs_distance = 0.005;
  feasible_measurements.zero_crossing_validation_sample_count = 20;
  const adasdf::SDFCreationReport feasible =
      adasdf::evaluateSDFCreationConstraints(
          constraints, feasible_measurements);
  if (!feasible.feasible() || !feasible.failed_constraints.empty()) {
    std::cerr << "feasible measurements were rejected\n";
    return 1;
  }

  adasdf::SDFCreationMeasurements exceeded;
  exceeded.sdf_file_bytes = 2048;
  exceeded.max_decoded_block_bytes = 768;
  exceeded.max_zero_surface_abs_error = 0.02;
  adasdf::SDFCreationAttempt attempt;
  attempt.attempt_index = 0;
  attempt.parameters.compression_method = "dense";
  attempt.measurements = exceeded;
  attempt.message = "measured after reload";
  const adasdf::SDFCreationReport infeasible =
      adasdf::evaluateSDFCreationConstraints(
          constraints, exceeded, {attempt});
  if (infeasible.status != adasdf::SDFCreationStatus::Infeasible ||
      infeasible.failed_constraints.size() != 3 ||
      infeasible.suggested_budget.max_sdf_file_bytes != 2048 ||
      infeasible.suggested_budget.max_decoded_block_bytes != 768 ||
      std::abs(infeasible.suggested_budget.max_zero_surface_abs_error - 0.02) >
          1.0e-15) {
    std::cerr << "infeasible report did not preserve all hard failures\n";
    return 1;
  }

  const std::string json = adasdf::toJson(infeasible);
  for (const std::string& field : {
           "\"schema_version\":1",
           "\"status\":\"Infeasible\"",
           "\"max_sdf_file_bytes\"",
           "\"max_decoded_block_bytes\"",
           "\"max_zero_surface_abs_error\"",
           "\"selected_attempt_index\":-1",
           "\"has_selected_backend_parameters\":false",
           "\"selected_backend_parameters\"",
           "\"deterministic_seed\"",
           "\"thread_count\"",
           "\"candidate_cost_order\"",
           "\"zero_surface_validation_complete\"",
           "\"zero_surface_validation_sample_count\"",
           "\"mesh_to_sdf_max_abs_phi\"",
           "\"sdf_to_mesh_max_abs_distance\"",
           "\"zero_crossing_validation_sample_count\"",
           "\"query_sample_count\"",
           "\"query_ns_per_sample\"",
           "\"exact_query_requests\"",
           "\"unique_exact_queries\"",
           "\"exact_query_cache_hits\"",
           "\"aggregate_oracle_stats\"",
           "\"advisor_source\"",
           "\"advisor_used_fallback\"",
           "\"attempts\"",
           "\"suggested_budget\""}) {
    if (json.find(field) == std::string::npos) {
      std::cerr << "missing stable JSON field: " << field << "\n";
      return 1;
    }
  }
  const std::string csv = adasdf::sdfCreationCsvHeader() + "\n" +
      adasdf::toCsvRow(infeasible);
  if (csv.find("max_zero_surface_abs_error_actual") == std::string::npos ||
      csv.find("selected_attempt_index") == std::string::npos ||
      csv.find("Infeasible") == std::string::npos) {
    std::cerr << "stable CSV contract is incomplete\n";
    return 1;
  }

  adasdf::SDFCreationMeasurements non_finite = feasible_measurements;
  non_finite.max_zero_surface_abs_error =
      std::numeric_limits<double>::infinity();
  const auto non_finite_report = adasdf::evaluateSDFCreationConstraints(
      constraints, non_finite);
  if (non_finite_report.status != adasdf::SDFCreationStatus::Infeasible) {
    std::cerr << "non-finite measured error must be infeasible\n";
    return 1;
  }
  const std::string non_finite_json = adasdf::toJson(non_finite_report);
  if (non_finite_json.find("\"max_zero_surface_abs_error\":null") ==
          std::string::npos ||
      non_finite_json.find(":inf") != std::string::npos ||
      non_finite_json.find(":nan") != std::string::npos) {
    std::cerr << "non-finite measurements must remain valid JSON\n";
    return 1;
  }

  return 0;
}

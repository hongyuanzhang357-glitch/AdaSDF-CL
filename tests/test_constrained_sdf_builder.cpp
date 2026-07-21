#include <adasdf/adasdf.h>

#include <filesystem>
#include <cmath>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

bool sameParameters(
    const adasdf::BackendBuildParameters& a,
    const adasdf::BackendBuildParameters& b) {
  return a.deterministic_seed == b.deterministic_seed &&
      a.thread_count == b.thread_count &&
      a.bvh_leaf_size == b.bvh_leaf_size &&
      a.octree_min_depth == b.octree_min_depth &&
      a.octree_max_depth == b.octree_max_depth &&
      a.octree_narrow_band_scale == b.octree_narrow_band_scale &&
      a.octree_interpolation_residual_scale ==
          b.octree_interpolation_residual_scale &&
      a.octree_max_overlapping_triangles ==
          b.octree_max_overlapping_triangles &&
      a.octree_max_normal_complexity == b.octree_max_normal_complexity &&
      a.block_min_nodes == b.block_min_nodes &&
      a.block_target_nodes == b.block_target_nodes &&
      a.block_max_nodes == b.block_max_nodes &&
      a.block_halo == b.block_halo &&
      a.block_min_tensor_dimension == b.block_min_tensor_dimension &&
      a.block_max_tensor_dimension == b.block_max_tensor_dimension &&
      a.compression_method == b.compression_method &&
      a.compression_abs_tolerance == b.compression_abs_tolerance &&
      a.compression_max_rank == b.compression_max_rank;
}

}  // namespace

int main() {
  const std::filesystem::path input =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";
  const std::filesystem::path dir =
      std::filesystem::path(ADASDF_CL_TEST_TEMP_DIR) /
      "constrained_sdf_builder";
  std::filesystem::create_directories(dir);
  const std::filesystem::path output = dir / "cube.sdfbin";
  adasdf::SDFCreationConstraints constraints;
  constraints.max_sdf_file_bytes = 64ull * 1024ull * 1024ull;
  constraints.max_decoded_block_bytes = 256ull * 1024ull;
  constraints.max_zero_surface_abs_error = 0.15;
  const adasdf::SDFCreationResult result =
      adasdf::ConstrainedSDFBuilder::fromSTL(input, output, constraints);
  if (!result.feasible() || !std::filesystem::exists(output) ||
      result.report.actual.sdf_file_bytes > constraints.max_sdf_file_bytes ||
      result.report.actual.max_decoded_block_bytes >
          constraints.max_decoded_block_bytes ||
      result.report.actual.max_zero_surface_abs_error >
          constraints.max_zero_surface_abs_error ||
      result.report.advisor_source != "heuristic-v1" ||
      result.report.advisor_used_fallback ||
      !result.report.actual.serialized_round_trip_verified ||
      !result.report.actual.zero_surface_validation_complete ||
      result.report.actual.zero_surface_validation_sample_count < 20 ||
      result.report.actual.zero_crossing_validation_sample_count == 0 ||
      result.report.actual.max_zero_surface_abs_error != std::max(
          result.report.actual.mesh_to_sdf_max_abs_phi,
          result.report.actual.sdf_to_mesh_max_abs_distance) ||
      result.report.attempts.size() < 2 ||
      result.report.selected_attempt_index < 0 ||
      !result.report.has_selected_backend_parameters) {
    std::cerr << adasdf::toJson(result.report) << "\n";
    return 1;
  }
  const adasdf::SDFCreationAttempt* selected_attempt = nullptr;
  std::size_t total_unique_queries = 0;
  std::size_t total_bvh_visits = 0;
  std::size_t total_triangle_tests = 0;
  std::size_t total_cache_hits = 0;
  for (const auto& attempt : result.report.attempts) {
    total_unique_queries += attempt.unique_exact_queries;
    total_bvh_visits += attempt.bvh_node_visits;
    total_triangle_tests += attempt.triangle_tests;
    total_cache_hits += attempt.exact_query_cache_hits;
    if (attempt.attempt_index == result.report.selected_attempt_index) {
      selected_attempt = &attempt;
    }
    if (!attempt.measurements.serialized_round_trip_verified ||
        !attempt.failed_constraints.empty()) {
      continue;
    }
    if (attempt.measurements.sdf_file_bytes <
        result.report.actual.sdf_file_bytes) {
      std::cerr << "selected candidate does not minimize feasible file bytes\n";
      return 1;
    }
  }
  if (selected_attempt == nullptr ||
      selected_attempt->measurements.sdf_file_bytes !=
          result.report.actual.sdf_file_bytes ||
      selected_attempt->query_sample_count != 1000 ||
      !(selected_attempt->query_ns_per_sample > 0.0) ||
      selected_attempt->exact_query_requests == 0 ||
      total_unique_queries == 0 || total_bvh_visits == 0 ||
      total_triangle_tests == 0 || total_cache_hits == 0) {
    std::cerr << "selected attempt evidence is inconsistent\n";
    return 1;
  }
  if (result.report.total_unique_exact_queries != total_unique_queries ||
      result.report.total_bvh_node_visits != total_bvh_visits ||
      result.report.total_triangle_tests != total_triangle_tests ||
      result.report.total_exact_query_cache_hits != total_cache_hits) {
    std::cerr << "aggregate oracle statistics are inconsistent\n";
    return 1;
  }

  const std::filesystem::path direct_output = dir / "cube_direct.sdfbin";
  const auto direct = adasdf::ConstrainedSDFBuilder::fromSTLWithParameters(
      input,
      direct_output,
      constraints,
      result.report.selected_backend_parameters);
  adasdf::ConstrainedSDFBinReport advisor_file;
  adasdf::ConstrainedSDFBinReport direct_file;
  adasdf::ConstrainedSDFBinReader::open(output, &advisor_file);
  adasdf::ConstrainedSDFBinReader::open(direct_output, &direct_file);
  if (!direct.feasible() || direct.report.advisor_source != "explicit-v1" ||
      direct.report.attempts.size() != 1 ||
      direct.report.selected_attempt_index != 0 ||
      !direct.report.has_selected_backend_parameters ||
      !sameParameters(
          result.report.selected_backend_parameters,
          direct.report.selected_backend_parameters) ||
      advisor_file.file_checksum != direct_file.file_checksum ||
      result.report.actual.sdf_file_bytes != direct.report.actual.sdf_file_bytes ||
      result.report.actual.max_decoded_block_bytes !=
          direct.report.actual.max_decoded_block_bytes ||
      result.report.actual.max_zero_surface_abs_error !=
          direct.report.actual.max_zero_surface_abs_error) {
    std::cerr << "direct parameters did not reproduce the selected asset\n"
              << adasdf::toJson(direct.report) << "\n";
    return 1;
  }
  for (const adasdf::Vector3 point : {
           adasdf::Vector3{0.5, 0.5, 0.5},
           adasdf::Vector3{0.0, 0.0, 0.0},
           adasdf::Vector3{1.0, 1.0, 1.0},
           adasdf::Vector3{0.25, 0.75, 0.4},
           adasdf::Vector3{1.2, 0.5, 0.5}}) {
    if (result.model->sampleDistance(point) !=
        direct.model->sampleDistance(point)) {
      std::cerr << "direct and selected assets disagree at a fixed query\n";
      return 1;
    }
  }

  adasdf::BackendBuildParameters invalid_parameters =
      result.report.selected_backend_parameters;
  invalid_parameters.thread_count = 2;
  const auto rejected_parameters =
      adasdf::ConstrainedSDFBuilder::fromSTLWithParameters(
          input,
          dir / "invalid_parameters.sdfbin",
          constraints,
          invalid_parameters);
  if (rejected_parameters.report.status !=
          adasdf::SDFCreationStatus::InvalidInput ||
      rejected_parameters.model ||
      std::filesystem::exists(dir / "invalid_parameters.sdfbin")) {
    std::cerr << "unsupported explicit thread count was not rejected\n";
    return 1;
  }
  const auto reloaded = adasdf::SDFBinReader::read(output);
  if (!reloaded || !reloaded->queryBackendAvailable() ||
      !std::isfinite(reloaded->sampleDistance({0.5, 0.5, 0.5}))) {
    std::cerr << "reloaded constrained SDF is not queryable\n";
    return 1;
  }
  const std::filesystem::path repeat_output = dir / "cube_repeat.sdfbin";
  const auto repeat = adasdf::ConstrainedSDFBuilder::fromSTL(
      input, repeat_output, constraints);
  adasdf::ConstrainedSDFBinReport first_file;
  adasdf::ConstrainedSDFBinReport repeat_file;
  adasdf::ConstrainedSDFBinReader::open(output, &first_file);
  adasdf::ConstrainedSDFBinReader::open(repeat_output, &repeat_file);
  if (!repeat.feasible() || repeat.report.advisor_used_fallback ||
      repeat.report.advisor_source != "heuristic-v1" ||
      !repeat.report.advisor_warnings.empty() ||
      first_file.file_checksum != repeat_file.file_checksum) {
    std::cerr << "constrained SDF build is not byte-deterministic\n";
    return 1;
  }

  const std::filesystem::path open_mesh =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "open_cube_missing_face_ascii.stl";
  const auto rejected_open = adasdf::ConstrainedSDFBuilder::fromSTL(
      open_mesh, dir / "open_mesh.sdfbin", constraints);
  if (rejected_open.report.status != adasdf::SDFCreationStatus::InvalidInput ||
      rejected_open.model || std::filesystem::exists(dir / "open_mesh.sdfbin")) {
    std::cerr << "non-watertight signed mesh was not rejected\n";
    return 1;
  }

  adasdf::TriangleMesh degenerate;
  degenerate.vertices = {{0.0, 0.0, 0.0},
                         {1.0, 0.0, 0.0},
                         {2.0, 0.0, 0.0}};
  degenerate.triangles = {{0, 1, 2, 0}};
  const auto rejected_degenerate = adasdf::ConstrainedSDFBuilder::fromMesh(
      degenerate, dir / "degenerate.sdfbin", constraints);
  if (rejected_degenerate.report.status !=
          adasdf::SDFCreationStatus::InvalidInput ||
      rejected_degenerate.model ||
      std::filesystem::exists(dir / "degenerate.sdfbin")) {
    std::cerr << "degenerate mesh was not rejected\n";
    return 1;
  }

  adasdf::SDFCreationConstraints impossible = constraints;
  impossible.max_decoded_block_bytes = 64;
  const auto infeasible = adasdf::ConstrainedSDFBuilder::fromSTL(
      input, dir / "impossible.sdfbin", impossible);
  if (infeasible.report.status != adasdf::SDFCreationStatus::Infeasible ||
      infeasible.model || infeasible.report.failed_constraints.empty() ||
      std::filesystem::exists(dir / "impossible.sdfbin")) {
    std::cerr << "impossible decoded-block budget was not structured Infeasible\n";
    return 1;
  }
  std::filesystem::remove_all(dir);
  return 0;
}

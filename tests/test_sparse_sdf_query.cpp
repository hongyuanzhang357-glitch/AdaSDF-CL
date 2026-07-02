#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
#endif

int main() {
  try {
    adasdf::DenseSDFBuildOptions build_options;
    build_options.resolution = 24;
    build_options.padding = 0.05;
    adasdf::DenseSDFBuildReport build_report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(
        (std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
         "closed_cube_ascii.stl")
            .string(),
        build_options,
        &build_report);
    if (!model || !model->queryBackendAvailable()) {
      std::cerr << "dense model build failed\n";
      return 1;
    }

    auto samples = adasdf::CollisionSampleSetIO::readCSV(
        (std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
         "cube_sparse_samples.csv")
            .string());
    if (!samples.success) {
      std::cerr << samples.error_message << "\n";
      return 1;
    }

    adasdf::SparseSDFQueryOptions options;
    options.compute_normals = false;
    auto result = adasdf::SparseSDFQuery::query(*model, samples.sample_set, options);
    if (!result.success || !result.colliding ||
        result.stats.queried_count != samples.sample_set.size()) {
      std::cerr << "basic sparse query failed\n";
      return 1;
    }
    if (result.samples.front().has_normal) {
      std::cerr << "phi-only query should not compute normals\n";
      return 1;
    }

    options.compute_normals = true;
    options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
    result = adasdf::SparseSDFQuery::query(*model, samples.sample_set, options);
    if (!result.samples.front().has_normal ||
        !result.samples.front().normal.allFinite()) {
      std::cerr << "normal query failed\n";
      return 1;
    }

    options.early_exit = true;
    options.compute_normals = false;
    options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
    result = adasdf::SparseSDFQuery::query(*model, samples.sample_set, options);
    if (!result.stats.early_exit_triggered || result.stats.queried_count >= samples.sample_set.size()) {
      std::cerr << "early exit did not reduce queried count\n";
      return 1;
    }

    auto radius_samples = adasdf::CollisionSampleSetIO::readCSV(
        (std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
         "cube_sparse_samples_with_radius.csv")
            .string());
    options = {};
    options.sort_results_by_effective_phi = true;
    result = adasdf::SparseSDFQuery::query(*model, radius_samples.sample_set, options);
    bool found_radius_hit = false;
    for (const auto& sample : result.samples) {
      if (sample.label == "radius_hits_surface" && sample.effective_phi <= 0.0) {
        found_radius_hit = true;
      }
    }
    if (!found_radius_hit) {
      std::cerr << "sample radius did not affect effective_phi\n";
      return 1;
    }
    std::cout << "sparse sdf query passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_sparse_sdf_query failed: " << exc.what() << "\n";
    return 1;
  }
}

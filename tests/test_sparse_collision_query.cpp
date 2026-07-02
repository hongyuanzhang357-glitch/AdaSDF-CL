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
  auto samples = adasdf::CollisionSampleSetIO::readCSV(
      (std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
       "cube_sparse_samples.csv")
          .string());
  if (!model || !samples.success) {
    return 1;
  }

  adasdf::SparseCollisionQueryOptions options;
  auto collision = adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
  if (!collision.success || !collision.colliding ||
      !collision.early_exit_triggered || collision.queried_count != 1) {
    std::cerr << "collision-only early exit failed\n";
    return 1;
  }
  if (!collision.violations.empty() && collision.violations.front().has_normal) {
    std::cerr << "collision-only should not compute normals by default\n";
    return 1;
  }

  options.mode = adasdf::SparseCollisionMode::Clearance;
  options.early_exit = false;
  collision = adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
  if (!collision.success || collision.queried_count != samples.sample_set.size() ||
      !(collision.min_effective_phi < 0.0)) {
    std::cerr << "clearance mode failed\n";
    return 1;
  }

  options.mode = adasdf::SparseCollisionMode::CandidateSearch;
  options.return_all_violations = true;
  options.compute_normals = true;
  collision = adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
  if (!collision.success || collision.violations.empty() ||
      !collision.violations.front().has_normal) {
    std::cerr << "candidate-search violations failed\n";
    return 1;
  }
  std::cout << "sparse collision query passed\n";
  return 0;
}

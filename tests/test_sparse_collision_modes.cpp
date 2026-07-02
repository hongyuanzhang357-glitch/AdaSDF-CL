#include <adasdf/adasdf.h>

#include <cmath>
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
       "cube_contact_candidates.csv")
          .string());
  if (!model || !samples.success) {
    return 1;
  }

  adasdf::SparseSDFQueryOptions phi_only;
  auto query = adasdf::SparseSDFQuery::query(*model, samples.sample_set, phi_only);
  if (!query.success || !std::isfinite(query.stats.min_effective_phi)) {
    return 1;
  }
  phi_only.compute_normals = true;
  phi_only.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
  query = adasdf::SparseSDFQuery::query(*model, samples.sample_set, phi_only);
  if (!query.success || !query.samples.front().has_normal) {
    return 1;
  }

  for (const auto mode : {
           adasdf::SparseCollisionMode::CollisionOnly,
           adasdf::SparseCollisionMode::Clearance,
           adasdf::SparseCollisionMode::CandidateSearch}) {
    adasdf::SparseCollisionQueryOptions options;
    options.mode = mode;
    options.return_all_violations = true;
    const auto collision =
        adasdf::SparseCollisionQuery::check(*model, samples.sample_set, options);
    if (!collision.success || !std::isfinite(collision.min_effective_phi)) {
      return 1;
    }
  }
  adasdf::ContactCandidateOptions reduce_options;
  reduce_options.top_k = 2;
  auto reduced =
      adasdf::ContactCandidateReducer::reduce(query.samples, reduce_options);
  if (reduced.candidates.size() > 2) {
    std::cerr << "top_k exceeded\n";
    return 1;
  }
  std::cout << "sparse collision modes passed\n";
  return 0;
}

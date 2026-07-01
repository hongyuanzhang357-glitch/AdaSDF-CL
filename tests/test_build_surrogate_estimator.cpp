#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

namespace {

adasdf::MeshFeatureSummary cubeFeatures() {
  adasdf::MeshFeatureSummary features;
  features.valid = true;
  features.vertex_count = 8;
  features.triangle_count = 12;
  features.aabb_diagonal = 1.7320508075688772;
  features.aabb_volume = 1.0;
  features.watertight = true;
  features.likely_oriented = true;
  features.connected_component_count = 1;
  features.readiness_score = 100;
  features.readiness_level = "Ready";
  return features;
}

}  // namespace

int main() {
  try {
    adasdf::BuildTarget target;
    target.target_near_surface_error = 1.0e-3;
    target.memory_budget_mb = 256.0;
    target.use_case = adasdf::BuildUseCase::MemorySaving;

    adasdf::BuildSurrogateEstimator estimator;
    const auto candidates =
        estimator.generateCandidates(cubeFeatures(), target);
    bool saw_dense = false;
    bool saw_adaptive = false;
    bool saw_compressed = false;
    bool saw_feasible = false;
    for (const adasdf::BuildCandidate& candidate : candidates) {
      const adasdf::BuildEstimate estimate =
          estimator.estimate(cubeFeatures(), target, candidate);
      if (!std::isfinite(estimate.estimated_memory_mb) ||
          !std::isfinite(estimate.estimated_near_surface_error) ||
          estimate.estimated_memory_mb < 0.0 ||
          estimate.estimated_near_surface_error < 0.0) {
        std::cerr << "non-finite estimate\n";
        return 1;
      }
      saw_feasible = saw_feasible || estimate.feasible;
      saw_dense = saw_dense ||
                  candidate.path == adasdf::RecommendedBuildPath::DenseSDF;
      saw_adaptive =
          saw_adaptive ||
          candidate.path == adasdf::RecommendedBuildPath::AdaptiveBlockSDF;
      saw_compressed =
          saw_compressed ||
          candidate.path ==
              adasdf::RecommendedBuildPath::CompressedAdaptiveBlockSDF;
    }
    if (candidates.empty() || !saw_dense || !saw_adaptive ||
        !saw_compressed || !saw_feasible) {
      std::cerr << "candidate generation missed required paths\n";
      return 1;
    }
    std::cout << "build surrogate estimator passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_surrogate_estimator failed: "
              << exc.what() << "\n";
    return 1;
  }
}

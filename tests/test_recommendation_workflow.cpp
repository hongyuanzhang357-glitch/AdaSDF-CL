#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    adasdf::BuildTarget target;
    target.target_near_surface_error = 1.0e-3;
    target.memory_budget_mb = 128.0;
    target.use_case = adasdf::BuildUseCase::MemorySaving;
    target.prefer_compression = true;

    adasdf::BuildRecommender recommender;
    const adasdf::BuildRecommendationReport report =
        recommender.recommendFromSTL(
            (fixture_dir / "closed_cube_ascii.stl").string(),
            target,
            "workflow_cube");
    if (!report.success ||
        report.recommended_recipe.path !=
            adasdf::RecommendedBuildPath::CompressedAdaptiveBlockSDF ||
        !contains(report.recommended_recipe.cli_command,
                  "adasdf_build_compressed_sdf") ||
        !contains(report.recommended_recipe.cli_command,
                  "--compression-report")) {
      std::cerr << "recommendation workflow did not select compressed path\n";
      return 1;
    }
    if (report.recommended_recipe.estimated_memory_mb >
        target.memory_budget_mb) {
      std::cerr << "recommendation exceeded memory target\n";
      return 1;
    }
    std::cout << "recommendation workflow passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_recommendation_workflow failed: "
              << exc.what() << "\n";
    return 1;
  }
}

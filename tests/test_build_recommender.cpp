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
    target.memory_budget_mb = 256.0;
    target.use_case = adasdf::BuildUseCase::Contact;
    target.prefer_compression = true;

    adasdf::BuildRecommender recommender;
    const adasdf::BuildRecommendationReport report =
        recommender.recommendFromSTL(
            (fixture_dir / "closed_cube_ascii.stl").string(),
            target,
            "closed_cube");
    if (!report.success || report.candidate_recipes.empty() ||
        report.limitations.empty()) {
      std::cerr << "recommendation report missing core fields\n";
      return 1;
    }
    if (!contains(report.recommended_recipe.cli_command, "adasdf_build_") ||
        !contains(report.limitations.front(), "experimental")) {
      std::cerr << "recommendation command or limitation missing\n";
      return 1;
    }
    bool saw_compressed = false;
    bool saw_adaptive = false;
    for (const adasdf::BuildRecipe& recipe : report.candidate_recipes) {
      saw_compressed =
          saw_compressed ||
          recipe.path ==
              adasdf::RecommendedBuildPath::CompressedAdaptiveBlockSDF;
      saw_adaptive =
          saw_adaptive ||
          recipe.path == adasdf::RecommendedBuildPath::AdaptiveBlockSDF;
    }
    if (!saw_compressed || !saw_adaptive) {
      std::cerr << "candidate report missing adaptive/compressed paths\n";
      return 1;
    }

    adasdf::BuildTarget open_target = target;
    open_target.allow_open_unsigned = true;
    const adasdf::BuildRecommendationReport open_report =
        recommender.recommendFromSTL(
            (fixture_dir / "open_cube_missing_face_ascii.stl").string(),
            open_target,
            "open_cube");
    if (!open_report.success ||
        !open_report.recommended_recipe.use_unsigned) {
      std::cerr << "open unsigned recommendation failed\n";
      return 1;
    }
    std::cout << "build recommender passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_recommender failed: " << exc.what() << "\n";
    return 1;
  }
}

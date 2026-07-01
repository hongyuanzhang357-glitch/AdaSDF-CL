#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

namespace {

adasdf::TriangleMesh makeCubeMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {
      {-0.5, -0.5, -0.5},
      {0.5, -0.5, -0.5},
      {0.5, 0.5, -0.5},
      {-0.5, 0.5, -0.5},
      {-0.5, -0.5, 0.5},
      {0.5, -0.5, 0.5},
      {0.5, 0.5, 0.5},
      {-0.5, 0.5, 0.5}};
  mesh.triangles = {
      {0, 1, 2, 0}, {0, 2, 3, 1}, {4, 6, 5, 2}, {4, 7, 6, 3},
      {0, 4, 5, 4}, {0, 5, 1, 5}, {1, 5, 6, 6}, {1, 6, 2, 7},
      {2, 6, 7, 8}, {2, 7, 3, 9}, {3, 7, 4, 10}, {3, 4, 0, 11}};
  return mesh;
}

}  // namespace

int main() {
  try {
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() / "adasdf_build_recommendation_demo";
    std::filesystem::create_directories(temp);
    const std::filesystem::path stl_path = temp / "cube_generated.stl";
    std::string write_error;
    if (!adasdf::STLWriter::write(
            stl_path.string(),
            makeCubeMesh(),
            {},
            &write_error)) {
      std::cerr << "failed to write demo STL: " << write_error << "\n";
      return 1;
    }

    adasdf::BuildTarget target;
    target.target_near_surface_error = 1.0e-3;
    target.memory_budget_mb = 256.0;
    target.use_case = adasdf::BuildUseCase::Contact;
    target.prefer_compression = true;

    adasdf::BuildRecommender recommender;
    const adasdf::BuildRecommendationReport report =
        recommender.recommendFromSTL(stl_path.string(), target, "demo_cube");
    if (!report.success) {
      std::cerr << "recommendation failed: " << report.error_message << "\n";
      return 1;
    }

    const adasdf::BuildRecipe& recipe = report.recommended_recipe;
    std::cout << "AdaSDF-CL build recommendation demo\n";
    std::cout << "Recommended path: " << adasdf::toString(recipe.path) << "\n";
    std::cout << "Confidence: " << adasdf::toString(recipe.confidence) << "\n";
    std::cout << "Estimated memory MB: " << recipe.estimated_memory_mb << "\n";
    std::cout << "Estimated near-surface error: "
              << recipe.estimated_near_surface_error << "\n";
    std::cout << "CLI command: " << recipe.cli_command << "\n";
    std::cout << "Build executed: no\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "20_build_recommendation_demo failed: " << exc.what() << "\n";
    return 1;
  }
}

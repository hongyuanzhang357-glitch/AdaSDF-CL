#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);

    adasdf::BuildTarget target;
    target.target_near_surface_error = 1.0e-3;
    target.memory_budget_mb = 256.0;
    adasdf::BuildRecommender recommender;
    const adasdf::BuildRecommendationReport report =
        recommender.recommendFromSTL(
            (fixture_dir / "closed_cube_ascii.stl").string(),
            target,
            "writer_cube");
    const std::string markdown =
        adasdf::BuildRecommendationWriter::toMarkdown(report);
    const std::string json = adasdf::BuildRecommendationWriter::toJson(report);
    if (!contains(markdown, "Recommended path") ||
        !contains(markdown, "CLI command") ||
        !contains(markdown, "not fully trained") ||
        !contains(json, "\"recommended_recipe\"") ||
        !contains(json, "not an optimality guarantee")) {
      std::cerr << "writer output missing required fields\n";
      return 1;
    }

    const auto md_path = temp / "recommendation_writer.md";
    const auto json_path = temp / "recommendation_writer.json";
    adasdf::BuildRecommendationWriter::writeMarkdown(md_path.string(), report);
    adasdf::BuildRecommendationWriter::writeJson(json_path.string(), report);
    if (!contains(readFile(md_path), "Candidate Table") ||
        !contains(readFile(json_path), "candidate_recipes")) {
      std::cerr << "writer files missing required content\n";
      return 1;
    }
    std::cout << "build recommendation writer passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_recommendation_writer failed: "
              << exc.what() << "\n";
    return 1;
  }
}

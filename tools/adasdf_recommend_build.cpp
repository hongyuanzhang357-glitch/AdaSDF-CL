#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_recommend_build input.stl "
         "[--target-error 1e-3] [--memory-mb 512] "
         "[--use-case contact|balanced|quality|memory-saving] "
         "[--signed|--unsigned] [--allow-open-unsigned] "
         "[--allow-dense-fallback|--no-dense-fallback] "
         "[--prefer-compression|--no-prefer-compression] "
         "[--prefer-fast-build] [--profile path] [--out recommendation.md] "
         "[--json recommendation.json] [--emit-command] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

bool anyFeasible(const adasdf::BuildRecommendationReport& report) {
  for (const adasdf::BuildRecipe& recipe : report.candidate_recipes) {
    if (recipe.estimates_feasible) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path markdown_path;
    std::filesystem::path json_path;
    std::filesystem::path profile_path;
    bool emit_command = false;
    bool verbose = false;
    adasdf::BuildTarget target;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        target.target_near_surface_error = std::stod(argv[++i]);
      } else if (arg == "--memory-mb" && hasValue(i, argc)) {
        target.memory_budget_mb = std::stod(argv[++i]);
      } else if (arg == "--use-case" && hasValue(i, argc)) {
        adasdf::BuildUseCase use_case;
        const std::string value = argv[++i];
        if (!adasdf::parseBuildUseCase(value, &use_case)) {
          std::cerr << "Unknown use case: " << value << "\n";
          usage();
          return 1;
        }
        target.use_case = use_case;
      } else if (arg == "--signed") {
        target.signed_distance = true;
      } else if (arg == "--unsigned") {
        target.signed_distance = false;
      } else if (arg == "--allow-open-unsigned") {
        target.allow_open_unsigned = true;
      } else if (arg == "--allow-dense-fallback") {
        target.allow_dense_fallback = true;
      } else if (arg == "--no-dense-fallback") {
        target.allow_dense_fallback = false;
      } else if (arg == "--prefer-compression") {
        target.prefer_compression = true;
      } else if (arg == "--no-prefer-compression") {
        target.prefer_compression = false;
      } else if (arg == "--prefer-fast-build") {
        target.prefer_fast_build = true;
      } else if (arg == "--profile" && hasValue(i, argc)) {
        profile_path = argv[++i];
      } else if (arg == "--out" && hasValue(i, argc)) {
        markdown_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--emit-command") {
        emit_command = true;
      } else if (arg == "--verbose") {
        verbose = true;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty()) {
      usage();
      return 1;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_recommend_build: input STL does not exist: "
                << input.string() << "\n";
      return 1;
    }
    if (!(target.target_near_surface_error > 0.0) ||
        !(target.memory_budget_mb > 0.0)) {
      std::cerr << "adasdf_recommend_build: target error and memory budget "
                   "must be positive\n";
      return 1;
    }

    adasdf::SurrogateProfileLoadResult profile =
        profile_path.empty()
            ? adasdf::SurrogateProfile::defaults()
            : adasdf::SurrogateProfile::load(profile_path.string());
    adasdf::BuildRecommenderOptions options;
    options.estimator_options = profile.options;
    adasdf::BuildRecommender recommender(options);
    const std::string output_basename = input.stem().string().empty()
                                            ? "model"
                                            : input.stem().string();
    adasdf::BuildRecommendationReport report =
        recommender.recommendFromSTL(input.string(), target, output_basename);
    for (const std::string& warning : profile.warnings) {
      report.global_warnings.push_back(warning);
    }

    if (!markdown_path.empty()) {
      adasdf::BuildRecommendationWriter::writeMarkdown(
          markdown_path.string(),
          report);
    }
    if (!json_path.empty()) {
      adasdf::BuildRecommendationWriter::writeJson(json_path.string(), report);
    }

    std::cout << "AdaSDF-CL build recommender\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Target near-surface error: "
              << target.target_near_surface_error << "\n";
    std::cout << "Memory budget MB: " << target.memory_budget_mb << "\n";
    std::cout << "Use case: " << adasdf::toString(target.use_case) << "\n";
    if (!report.success) {
      std::cout << "Recommendation: failed\n";
      std::cout << "Error: " << report.error_message << "\n";
      return 1;
    }

    const adasdf::BuildRecipe& recipe = report.recommended_recipe;
    std::cout << "Recommended path: " << adasdf::toString(recipe.path) << "\n";
    std::cout << "Confidence: " << adasdf::toString(recipe.confidence) << "\n";
    std::cout << "Estimated memory MB: " << recipe.estimated_memory_mb << "\n";
    std::cout << "Estimated near-surface error: "
              << recipe.estimated_near_surface_error << "\n";
    std::cout << "Estimated compression ratio: "
              << recipe.estimated_compression_ratio << "\n";
    std::cout << "CLI command: " << recipe.cli_command << "\n";
    std::cout << "Build executed: no\n";
    std::cout << "Model boundary: experimental deterministic recommender; "
                 "not a universal trained model; not fully trained; not an "
                 "optimality guarantee\n";
    if (!markdown_path.empty()) {
      std::cout << "Markdown report: " << markdown_path.string() << "\n";
    }
    if (!json_path.empty()) {
      std::cout << "JSON report: " << json_path.string() << "\n";
    }
    if (emit_command) {
      std::cout << recipe.cli_command << "\n";
    }
    if (verbose) {
      for (const std::string& warning : report.global_warnings) {
        std::cout << "Warning: " << warning << "\n";
      }
      for (const std::string& warning : recipe.warnings) {
        std::cout << "Warning: " << warning << "\n";
      }
    }
    if (!anyFeasible(report)) {
      std::cerr << "adasdf_recommend_build: no candidate fully meets the "
                   "requested target\n";
      return 2;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_recommend_build failed: " << exc.what() << "\n";
    return 1;
  }
}

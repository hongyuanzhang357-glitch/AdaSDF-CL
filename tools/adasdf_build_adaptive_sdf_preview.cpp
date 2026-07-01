#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_adaptive_sdf_preview input.stl output.sdfbin "
         "[--target-error 1e-3] [--far-error 1e-2] [--memory-mb 512] "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--max-rank N] [--enable-low-rank] [--dry-run] [--plan plan.md]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  file << text;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path plan_path;
    bool dry_run = false;
    adasdf::AdaptiveSDFBuildOptions options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.target_near_surface_error = std::stod(argv[++i]);
      } else if (arg == "--far-error" && hasValue(i, argc)) {
        options.target_far_field_error = std::stod(argv[++i]);
      } else if (arg == "--memory-mb" && hasValue(i, argc)) {
        options.memory_budget_mb = std::stod(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--enable-low-rank") {
        options.enable_low_rank_compression = true;
      } else if (arg == "--dry-run") {
        dry_run = true;
      } else if (arg == "--plan" && hasValue(i, argc)) {
        plan_path = argv[++i];
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else if (output.empty()) {
        output = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty() || output.empty()) {
      usage();
      return 1;
    }
    const adasdf::AdaptiveSDFBuildPlan plan =
        adasdf::AdaptiveSDFBuilderPreview::makePlan(options);
    const std::string markdown =
        adasdf::AdaptiveSDFBuilderPreview::planToMarkdown(plan);
    if (!plan_path.empty()) {
      writeText(plan_path, markdown);
    }

    std::cout << "AdaSDF-CL adaptive builder interface preview\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Requested output: " << output.string() << "\n";
    std::cout << "Dry run: " << (dry_run ? "yes" : "no") << "\n";
    std::cout << "Valid plan: " << (plan.valid ? "yes" : "no") << "\n";
    std::cout << "Implemented in this version: "
              << (plan.implemented_in_this_version ? "yes" : "no") << "\n";
    std::cout
        << "Adaptive octree/block SDF construction is implemented in "
           "v1.6.0-alpha as block-wise dense output.\n";
    std::cout
        << "Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF "
           "construction.\n";
    std::cout
        << "Low-rank block compression is implemented in v1.7.0-alpha using "
           "matrix-SVD per adaptive block.\n";
    std::cout
        << "Use adasdf_build_compressed_sdf for one-step compressed output.\n";
    std::cout
        << "Tucker/HOSVD compression and surrogate recommendation remain "
           "planned.\n";
    if (!plan_path.empty()) {
      std::cout << "Plan: " << plan_path.string() << "\n";
    }

    if (!dry_run) {
      return 3;
    }
    return plan.valid ? 0 : 2;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_adaptive_sdf_preview failed: " << exc.what()
              << "\n";
    return 1;
  }
}

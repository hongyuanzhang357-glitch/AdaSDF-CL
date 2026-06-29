#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_recommend_demo --shape box --target-error value "
      << "--memory-mb value --block-memory-mb value --top-k value\n";
}

bool hasValue(int index, int argc, int count = 1) {
  return index + count < argc;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }

  DemoSurrogateInput input;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--shape" && hasValue(i, argc)) {
      input.shape_type = argv[++i];
    } else if (arg == "--target-error" && hasValue(i, argc)) {
      input.target_near_surface_error = std::stod(argv[++i]);
    } else if (arg == "--memory-mb" && hasValue(i, argc)) {
      input.memory_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--block-memory-mb" && hasValue(i, argc)) {
      input.block_expand_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--top-k" && hasValue(i, argc)) {
      input.top_k = std::stoi(argv[++i]);
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  try {
    const auto candidates = DemoSurrogateRecommender::recommend(input);
    std::cout << "AdaSDF-CL demo surrogate recommender\n";
    std::cout << "Status: experimental demo only\n";
    std::cout << "Warning: "
              << DemoSurrogateRecommender::statusWarning() << "\n";
    std::cout << "Surrogate: " << DemoSurrogateRecommender::id() << "\n";
    std::cout << "Shape: " << input.shape_type << "\n";
    std::cout << "Requested top-k: " << input.top_k << "\n";
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      const auto& candidate = candidates[i];
      std::cout << "\nCandidate[" << i << "]\n";
      std::cout << "  target error: "
                << candidate.options.near_surface_error << "\n";
      std::cout << "  predicted p95 error: "
                << candidate.predicted_p95_error << "\n";
      std::cout << "  predicted memory: "
                << candidate.predicted_memory_mb << " MB\n";
      std::cout << "  confidence: " << candidate.confidence << "\n";
      std::cout << "  max octree level: "
                << candidate.options.max_octree_level << "\n";
      std::cout << "  block resolution: "
                << candidate.options.base_block_cells << "\n";
      std::cout << "  rank: " << candidate.options.max_rank << "\n";
      std::cout << "  compression: "
                << candidate.options.compression_method << "\n";
      std::cout << "  warning: " << candidate.warning << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_recommend_demo failed: " << exc.what() << "\n";
    return 1;
  }
}

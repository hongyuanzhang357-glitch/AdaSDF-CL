#include <adasdf/adasdf.h>

#include <iostream>
#include <string>

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    adasdf::DemoSurrogateInput input;
    input.top_k = 5;
    const auto candidates = adasdf::DemoSurrogateRecommender::recommend(input);
    if (candidates.size() != 5) {
      std::cerr << "demo recommender did not return top-k candidates\n";
      return 1;
    }
    if (!contains(candidates.front().warning, "not universal") ||
        !contains(candidates.front().warning, "demo")) {
      std::cerr << "demo recommender warning is missing required boundary text\n";
      return 1;
    }

    adasdf::DemoSurrogateInput coarse;
    coarse.target_near_surface_error = 1.0e-2;
    adasdf::DemoSurrogateInput fine;
    fine.target_near_surface_error = 1.0e-4;
    const auto coarse_candidate =
        adasdf::DemoSurrogateRecommender::recommend(coarse).front();
    const auto fine_candidate =
        adasdf::DemoSurrogateRecommender::recommend(fine).front();
    if (fine_candidate.options.base_block_cells <
            coarse_candidate.options.base_block_cells ||
        fine_candidate.options.max_rank < coarse_candidate.options.max_rank) {
      std::cerr << "smaller target error should increase resolution or rank\n";
      return 1;
    }

    adasdf::DemoSurrogateInput small_memory;
    small_memory.memory_limit_mb = 16.0;
    adasdf::DemoSurrogateInput normal_memory;
    normal_memory.memory_limit_mb = 64.0;
    const auto small_candidate =
        adasdf::DemoSurrogateRecommender::recommend(small_memory).front();
    const auto normal_candidate =
        adasdf::DemoSurrogateRecommender::recommend(normal_memory).front();
    if (small_candidate.options.max_rank > normal_candidate.options.max_rank ||
        small_candidate.predicted_memory_mb > small_memory.memory_limit_mb) {
      std::cerr << "small memory limit should constrain rank and predicted memory\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_demo_surrogate_recommender failed: "
              << exc.what() << "\n";
    return 1;
  }
}

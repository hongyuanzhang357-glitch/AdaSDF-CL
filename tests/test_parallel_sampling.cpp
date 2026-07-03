#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <vector>

int main() {
  try {
    std::vector<int> values(128, 0);
    adasdf::ParallelSamplingOptions options;
    options.threads = 4;
    const adasdf::ParallelSamplingStats stats =
        adasdf::parallelFor(values.size(), options, [&](std::size_t i) {
          values[i] = static_cast<int>(i * i);
        });
    if (stats.item_count != values.size() || stats.threads_used < 1) {
      std::cerr << "parallel stats failed\n";
      return 1;
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (values[i] != static_cast<int>(i * i)) {
        std::cerr << "parallel deterministic fill failed\n";
        return 1;
      }
    }
    std::cout << "parallel sampling passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_parallel_sampling failed: " << exc.what() << "\n";
    return 1;
  }
}

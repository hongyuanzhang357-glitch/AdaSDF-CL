#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

int main() {
  adasdf::BuildOptions options;
  if (std::abs(options.near_surface_error - options.tau_near_abs) > 1.0e-15) {
    std::cerr << "near_surface_error and tau_near_abs defaults diverged\n";
    return 1;
  }
  if (static_cast<double>(options.max_memory_mb) != options.memory_limit_mb) {
    std::cerr << "max_memory_mb and memory_limit_mb defaults diverged\n";
    return 1;
  }
  if (static_cast<double>(options.block_expand_limit_mb) !=
      options.block_memory_limit_mb) {
    std::cerr << "block memory defaults diverged\n";
    return 1;
  }
  if (options.backend != adasdf::BackendType::CPU ||
      options.query_mode != adasdf::QueryMode::Balanced) {
    std::cerr << "BuildOptions backend/query defaults are unexpected\n";
    return 1;
  }
  return 0;
}

#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::CudaActiveBlockQueryOptions query_options;
  query_options.output_mode = adasdf::CudaActiveBlockOutputMode::PhiOnly;
  adasdf::CudaActiveBlockBenchmarkOptions benchmark_options;
  benchmark_options.query_options = query_options;
  if (benchmark_options.repeat <= 0 || benchmark_options.warmup < 0) {
    std::cerr << "CUDA active block Python contract defaults are invalid\n";
    return 1;
  }
  std::cout << "CUDA active block Python contract compile check passed\n";
  return 0;
}

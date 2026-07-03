#include "adasdf/acceleration/ParallelSampling.h"

#include <algorithm>
#include <thread>

namespace adasdf {

int normalizedThreadCount(int requested, std::size_t item_count) {
  if (item_count == 0) {
    return 0;
  }
  if (requested <= 1) {
    return 1;
  }
  unsigned int hardware = std::thread::hardware_concurrency();
  if (hardware == 0) {
    hardware = 1;
  }
  const int capped = std::min<int>(requested, static_cast<int>(hardware));
  return std::max(1, std::min<int>(capped, static_cast<int>(item_count)));
}

}  // namespace adasdf

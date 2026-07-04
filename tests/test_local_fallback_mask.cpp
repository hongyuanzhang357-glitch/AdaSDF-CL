#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

int main() {
  std::vector<double> phi(64, 1.0);
  phi[1 + 4 * (1 + 4 * 1)] = 0.01;
  const adasdf::LocalFallbackMask mask =
      adasdf::LocalFallbackMaskBuilder::build(4, 4, 4, phi, 0.05, 1);
  if (!mask.valid() || !mask.isExactRequired(0, 0, 0) ||
      !mask.isExactRequired(1, 1, 1) || mask.isExactRequired(2, 2, 2) ||
      mask.exactRequiredCount() <= 56) {
    std::cerr << "local fallback mask did not mark halo/near-band nodes\n";
    return 1;
  }
  std::cout << "local fallback mask passed\n";
  return 0;
}

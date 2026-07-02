#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::ActiveExpandedBlock block;
    block.block_id = 7;
    block.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
    block.nx = 2;
    block.ny = 2;
    block.nz = 2;
    block.origin = {0.0, 0.0, 0.0};
    block.spacing = {1.0, 1.0, 1.0};
    block.phi = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    if (!block.isValid() || !block.contains({0.5, 0.5, 0.5})) {
      std::cerr << "expanded block validity failed\n";
      return 1;
    }
    const double phi = block.sampleDistance({0.5, 0.5, 0.5});
    if (!std::isfinite(phi) || std::abs(phi - 0.5) > 1.0e-9 ||
        !block.sampleGradient({0.5, 0.5, 0.5}).allFinite()) {
      std::cerr << "expanded block sampling failed\n";
      return 1;
    }
    std::cout << "expanded block passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_expanded_block failed: " << exc.what() << "\n";
    return 1;
  }
}

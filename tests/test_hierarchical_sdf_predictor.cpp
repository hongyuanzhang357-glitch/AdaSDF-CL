#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

std::size_t idx(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

}  // namespace

int main() {
  const adasdf::AABB bounds{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  std::vector<double> coarse(8, 0.0);
  for (int k = 0; k < 2; ++k) {
    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < 2; ++i) {
        coarse[idx(i, j, k, 2, 2)] =
            static_cast<double>(i + j + k);
      }
    }
  }

  const auto prediction =
      adasdf::HierarchicalSDFPredictor::predictFromCoarseSamples(
          bounds,
          3,
          3,
          3,
          coarse,
          2,
          2,
          2);
  if (!prediction.success || prediction.predicted_phi.size() != 27) {
    std::cerr << "coarse prediction failed\n";
    return 1;
  }
  const double center = prediction.predicted_phi[idx(1, 1, 1, 3, 3)];
  if (std::abs(center - 1.5) > 1e-12) {
    std::cerr << "unexpected center interpolation: " << center << "\n";
    return 1;
  }

  std::cout << "hierarchical sdf predictor passed\n";
  return 0;
}

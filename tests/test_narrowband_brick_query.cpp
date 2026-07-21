#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

std::size_t idx(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::AdaptiveBlockSDFModel makeModel() {
  adasdf::AdaptiveSDFBlockSet set;
  set.global_bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, true};
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 0;
  block.octree_node_id = 0;
  block.level = 0;
  block.bounds = set.global_bounds;
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.origin = block.bounds.min;
  block.spacing = {0.5, 0.5, 0.5};
  block.phi.assign(27, 0.0);
  for (int k = 0; k < 3; ++k) {
    for (int j = 0; j < 3; ++j) {
      for (int i = 0; i < 3; ++i) {
        block.phi[idx(i, j, k, 3, 3)] =
            0.5 * static_cast<double>(i + j + k);
      }
    }
  }
  set.blocks.push_back(block);
  return adasdf::AdaptiveBlockSDFModel(set);
}

}  // namespace

int main() {
  const adasdf::AdaptiveBlockSDFModel model = makeModel();
  adasdf::NarrowBandBrickIndex index;
  if (!index.build(model)) {
    std::cerr << "failed to build index\n";
    return 1;
  }
  adasdf::NarrowBandBrickQueryStats stats;
  const adasdf::Vector3 p{0.25, 0.50, 0.75};
  const auto fast = adasdf::NarrowBandBrickQuery::samplePhi(index, p, &stats);
  const double legacy = model.sampleDistance(p);
  if (!fast.success || std::abs(fast.phi - legacy) > 1.0e-12 ||
      std::abs(fast.phi - 1.5) > 1.0e-12 ||
      stats.averageTensorNodesTouchedPerQuery() != 8.0) {
    std::cerr << "brick-fast query does not match legacy interpolation\n";
    return 1;
  }
  std::cout << "narrow-band brick query passed\n";
  return 0;
}


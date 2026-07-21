#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::SamplingOctreePlan sampling;
    sampling.bounds.valid = true;
    sampling.bounds.min = {0.0, 0.0, 0.0};
    sampling.bounds.max = {1.0, 1.0, 1.0};
    for (int i = 0; i < 8; ++i) {
      adasdf::SamplingOctreeNode node;
      node.node_id = i;
      node.level = 3 + (i % 2);
      node.bounds.valid = true;
      node.bounds.min = {
          (i & 1) ? 0.50 : 0.00,
          (i & 2) ? 0.50 : 0.00,
          (i & 4) ? 0.50 : 0.00};
      node.bounds.max = {
          node.bounds.min.x + 0.25,
          node.bounds.min.y + 0.25,
          node.bounds.min.z + 0.25};
      node.contact_band = (i == 0);
      node.near_zero_surface = node.contact_band;
      sampling.nodes.push_back(node);
    }

    adasdf::NarrowBandBrickBuildOptions options;
    std::string error;
    if (!adasdf::parseBrickLevelMap("0-4:1", &options.brick_level_map, &error)) {
      std::cerr << error << "\n";
      return 1;
    }
    options.max_sampling_level = 4;
    const adasdf::CompressionBrickTreePlan bricks =
        adasdf::CompressionBrickTree::build(sampling, options);
    if (bricks.bricks.size() != 8 || sampling.nodes.size() != bricks.bricks.size()) {
      std::cerr << "test fixture should start with equal counts\n";
      return 1;
    }
    bool saw_contact = false;
    for (const adasdf::CompressionBrick& brick : bricks.bricks) {
      if (brick.covered_sampling_node_ids.empty()) {
        std::cerr << "mapper failed to attach sampling node to brick\n";
        return 1;
      }
      if (brick.contact_band_node_count > 0) {
        saw_contact = true;
      }
      if (brick.coarsest_sampling_level_inside <= 0 ||
          brick.finest_sampling_level_inside <
              brick.coarsest_sampling_level_inside) {
        std::cerr << "mapper did not preserve mixed-level sampling metadata\n";
        return 1;
      }
    }
    if (!saw_contact) {
      std::cerr << "contact sampling demand did not reach any brick\n";
      return 1;
    }
    std::cout << "sampling-to-brick mapper passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_sampling_to_brick_mapper failed: " << exc.what()
              << "\n";
    return 1;
  }
}

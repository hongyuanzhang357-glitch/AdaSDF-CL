#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::NarrowBandBrickBuildOptions options;
    std::string error;
    if (!adasdf::parseBrickLevelMap(
            "0-2:0,3-5:1,6-8:2",
            &options.brick_level_map,
            &error)) {
      std::cerr << error << "\n";
      return 1;
    }
    if (adasdf::brickLevelForSamplingLevel(options.brick_level_map, 2) != 0 ||
        adasdf::brickLevelForSamplingLevel(options.brick_level_map, 5) != 1 ||
        adasdf::brickLevelForSamplingLevel(options.brick_level_map, 7) != 2) {
      std::cerr << "brick-level-map lookup failed\n";
      return 1;
    }

    adasdf::SamplingOctreePlan sampling;
    sampling.bounds.valid = true;
    sampling.bounds.min = {0.0, 0.0, 0.0};
    sampling.bounds.max = {1.0, 1.0, 1.0};
    options.max_sampling_level = 6;
    options.brick_target_tensor_dim = 48;
    options.brick_max_tensor_dim = 32;
    const adasdf::CompressionBrickTreePlan plan =
        adasdf::CompressionBrickTree::build(sampling, options);
    if (plan.bricks.size() != 64 ||
        plan.brick_count_by_level.at(2) != 64 ||
        plan.bricks.front().tensor_nx > 32 ||
        plan.bricks.front().tensor_node_count == 0 ||
        plan.tensor_dim_distribution.empty()) {
      std::cerr << "compression brick tree planning failed\n";
      return 1;
    }
    if (plan.bricks.front().coarsest_sampling_level_inside >
        plan.bricks.front().finest_sampling_level_inside) {
      std::cerr << "brick sampling level range is invalid\n";
      return 1;
    }
    std::cout << "compression brick tree passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_compression_brick_tree failed: " << exc.what() << "\n";
    return 1;
  }
}

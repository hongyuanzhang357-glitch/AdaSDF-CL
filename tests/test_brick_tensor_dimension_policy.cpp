#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::NarrowBandBrickBuildOptions options;
    options.brick_min_tensor_dim = 8;
    options.brick_target_tensor_dim = 24;
    options.brick_max_tensor_dim = 32;
    options.brick_max_expanded_kb = 512;
    options.contact_exact_min_node_ratio = 0.10;

    adasdf::CompressionBrick contact;
    contact.covered_sampling_node_ids = {1, 2, 3};
    contact.contact_band_node_count = 2;
    const adasdf::BrickTensorDimensionDecision contact_decision =
        adasdf::BrickTensorDimensionPolicy::decide(contact, options);
    if (contact_decision.tensor_nx != 24 ||
        contact_decision.estimated_exact_source_node_count == 0 ||
        contact_decision.estimated_interpolated_fill_node_count == 0) {
      std::cerr << "contact brick did not use target tensor demand\n";
      return 1;
    }

    adasdf::CompressionBrick far;
    far.covered_sampling_node_ids = {4};
    far.far_field_node_count = 1;
    options.brick_merge_small = true;
    const adasdf::BrickTensorDimensionDecision far_decision =
        adasdf::BrickTensorDimensionPolicy::decide(far, options);
    if (far_decision.tensor_nx != 8 || !far_decision.should_merge ||
        far_decision.estimated_exact_source_node_count != 0) {
      std::cerr << "far-field brick did not stay coarse/mergeable\n";
      return 1;
    }

    options.brick_min_tensor_dim = 32;
    options.brick_target_tensor_dim = 64;
    options.brick_max_tensor_dim = 64;
    options.brick_max_expanded_kb = 64;
    const adasdf::BrickTensorDimensionDecision limited =
        adasdf::BrickTensorDimensionPolicy::decide(contact, options);
    if (!limited.should_split || !limited.warning_too_large) {
      std::cerr << "memory-constrained brick did not request split\n";
      return 1;
    }
    std::cout << "brick tensor dimension policy passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_brick_tensor_dimension_policy failed: " << exc.what()
              << "\n";
    return 1;
  }
}

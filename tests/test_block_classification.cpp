#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::AdaptiveSDFBlock makeBlock(std::initializer_list<double> phi) {
  adasdf::AdaptiveSDFBlock block;
  block.nx = 2;
  block.ny = 2;
  block.nz = 1;
  block.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, true};
  block.phi.assign(phi.begin(), phi.end());
  return block;
}

}  // namespace

int main() {
  adasdf::BlockClassificationOptions options;
  options.near_surface_band = 0.1;
  options.transition_band = 0.5;
  options.use_block_diagonal_scale = false;
  options.near_surface_diagonal_factor = 1.0;
  options.transition_diagonal_factor = 1.0;

  const auto near_surface =
      adasdf::BlockClassifier::classify(makeBlock({-0.2, 0.2, 0.3, 0.4}), options);
  if (near_surface.importance != adasdf::BlockImportanceClass::NearSurface ||
      !near_surface.has_sign_change) {
    std::cerr << "sign-changing block should be near surface\n";
    return 1;
  }

  const auto transition =
      adasdf::BlockClassifier::classify(makeBlock({0.2, 0.3, 0.4, 0.45}), options);
  if (transition.importance != adasdf::BlockImportanceClass::Transition) {
    std::cerr << "moderate-distance block should be transition\n";
    return 1;
  }

  const auto far_field =
      adasdf::BlockClassifier::classify(makeBlock({1.0, 1.1, 1.2, 1.3}), options);
  if (far_field.importance != adasdf::BlockImportanceClass::FarField) {
    std::cerr << "far block should be far field\n";
    return 1;
  }

  std::cout << "block classification passed\n";
  return 0;
}

#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::QuantizationOptions options;
  options.origin = {0.0, 0.0, 0.0};
  options.spacing = 0.25;
  options.epsilon = 1e-9;

  const adasdf::QuantizedPointKey a =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {1.0, 0.5, -0.25},
          3,
          options);
  const adasdf::QuantizedPointKey b =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {1.0 + 1e-13, 0.5 - 1e-13, -0.25 + 1e-13},
          3,
          options);
  if (!(a == b)) {
    std::cerr << "near-identical points should quantize to the same key\n";
    return 1;
  }

  const adasdf::QuantizedPointKey shared_corner =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {0.25, 0.25, 0.25},
          2,
          options);
  const adasdf::QuantizedPointKey adjacent_block_corner =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {0.25 + 1e-13, 0.25, 0.25},
          2,
          options);
  if (!(shared_corner == adjacent_block_corner)) {
    std::cerr << "shared block corner should quantize identically\n";
    return 1;
  }

  const adasdf::QuantizedPointKey level3 =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {0.25, 0.25, 0.25},
          3,
          options);
  if (shared_corner == level3) {
    std::cerr << "level-aware keys should differ across levels\n";
    return 1;
  }

  options.include_level = false;
  const adasdf::QuantizedPointKey no_level2 =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {0.25, 0.25, 0.25},
          2,
          options);
  const adasdf::QuantizedPointKey no_level3 =
      adasdf::QuantizedPointKeyBuilder::fromPoint(
          {0.25, 0.25, 0.25},
          3,
          options);
  if (!(no_level2 == no_level3)) {
    std::cerr << "level-disabled keys should match across levels\n";
    return 1;
  }
  return 0;
}

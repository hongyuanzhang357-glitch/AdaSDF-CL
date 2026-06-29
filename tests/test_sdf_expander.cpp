#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool closeEnough(double a, double b, double tolerance) {
  return std::abs(a - b) <= tolerance;
}

}  // namespace

int main() {
  const auto analytic = adasdf::AnalyticSDFModel::createBox();
  const adasdf::ExpandedSDF global =
      adasdf::SDFExpander::expandGlobal(*analytic, 32);
  if (!global.isValid() || global.layout() != adasdf::ExpandedSDFLayout::GlobalDense ||
      !global.hasBlock(0) || global.memoryFootprintBytes() == 0) {
    std::cerr << "Global expansion is invalid\n";
    return 1;
  }
  const adasdf::Vector3 p{0.2, 0.1, 0.0};
  if (!closeEnough(global.sampleDistance(p), analytic->sampleDistance(p), 0.04)) {
    std::cerr << "Global expanded sample is too far from analytic sample\n";
    return 1;
  }

  adasdf::DemoAdaptiveBuildRequest request;
  request.use_surrogate = false;
  const auto demo = adasdf::DemoAdaptiveSDFBuilder::build(request);
  const adasdf::ExpandedSDF block0 = adasdf::SDFExpander::expandBlocks(
      *demo.model,
      adasdf::BlockSelection::selected({0}),
      16);
  if (!block0.isValid() || block0.layout() != adasdf::ExpandedSDFLayout::BlockDense ||
      !block0.hasBlock(0) || block0.hasBlock(1)) {
    std::cerr << "Selected block expansion did not keep only block 0\n";
    return 1;
  }

  try {
    (void)adasdf::SDFExpander::expandBlocks(
        *demo.model,
        adasdf::BlockSelection::selected({999}),
        16);
    std::cerr << "Invalid block id did not throw\n";
    return 1;
  } catch (const std::runtime_error& error) {
    if (std::string(error.what()).find("Requested block id does not exist") ==
        std::string::npos) {
      std::cerr << "Unexpected invalid block id error: " << error.what() << "\n";
      return 1;
    }
  }

  std::cout << "SDFExpander tests passed\n";
  return 0;
}

#include <adasdf/adasdf.h>

#include <iostream>

namespace {

adasdf::ExpansionQualityReport qualityAtResolution(int resolution) {
  const auto model = adasdf::AnalyticSDFModel::createBox();
  adasdf::ExpansionOptions expansion_options;
  expansion_options.expansion = adasdf::QueryExpansionMode::Global;
  expansion_options.global_resolution = resolution;
  const adasdf::ExpandedSDF expanded =
      adasdf::SDFExpander::expand(*model, expansion_options);

  adasdf::ExpansionQualityOptions quality_options;
  quality_options.num_samples = 512;
  quality_options.near_surface_band = 0.05;
  quality_options.seed = 42;
  return adasdf::ExpansionQuality::compareAgainstDirect(
      *model,
      expanded,
      quality_options);
}

}  // namespace

int main() {
  try {
    const adasdf::ExpansionQualityReport coarse = qualityAtResolution(16);
    const adasdf::ExpansionQualityReport fine = qualityAtResolution(32);
    if (fine.mean_abs_error > coarse.mean_abs_error + 0.02) {
      std::cerr << "higher resolution made mean error clearly worse\n";
      return 1;
    }
    if (fine.p95_abs_error > coarse.p95_abs_error + 0.02) {
      std::cerr << "higher resolution made p95 error clearly worse\n";
      return 1;
    }
    if (fine.num_finite_samples != coarse.num_finite_samples) {
      std::cerr << "resolution comparison sample counts changed\n";
      return 1;
    }

    std::cout << "expansion resolution control passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_expansion_resolution_control failed: "
              << exc.what() << "\n";
    return 1;
  }
}

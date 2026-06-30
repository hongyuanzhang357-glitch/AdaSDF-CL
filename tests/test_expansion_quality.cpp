#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>

namespace {

bool rateValid(double value) {
  return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

}  // namespace

int main() {
  try {
    const auto model = adasdf::AnalyticSDFModel::createBox();
    adasdf::ExpansionOptions expansion_options;
    expansion_options.expansion = adasdf::QueryExpansionMode::Global;
    expansion_options.global_resolution = 32;
    expansion_options.near_surface_band = 0.05;
    expansion_options.sign_epsilon = 1.0e-9;
    const adasdf::ExpandedSDF expanded =
        adasdf::SDFExpander::expand(*model, expansion_options);

    adasdf::ExpansionQualityOptions quality_options;
    quality_options.num_samples = 512;
    quality_options.near_surface_band = 0.05;
    quality_options.sign_epsilon = 1.0e-9;
    quality_options.seed = 17;
    const adasdf::ExpansionQualityReport report =
        adasdf::ExpansionQuality::compareAgainstDirect(
            *model,
            expanded,
            quality_options);

    if (report.num_samples != 512 || report.num_finite_samples == 0) {
      std::cerr << "quality report sample counts are invalid\n";
      return 1;
    }
    if (!std::isfinite(report.max_abs_error) ||
        !std::isfinite(report.mean_abs_error) ||
        !std::isfinite(report.rms_error) ||
        !std::isfinite(report.p95_abs_error)) {
      std::cerr << "quality errors are not finite\n";
      return 1;
    }
    if (report.p95_abs_error > 0.08 || report.max_abs_error > 0.12) {
      std::cerr << "expanded quality error is unexpectedly high\n";
      return 1;
    }
    if (!rateValid(report.sign_mismatch_rate) ||
        !rateValid(report.ambiguous_sign_rate) ||
        !rateValid(report.near_surface_sign_mismatch_rate) ||
        !rateValid(report.fallback_rate)) {
      std::cerr << "quality rates are invalid\n";
      return 1;
    }
    if (report.worst_point_id < 0 || !report.worst_point.allFinite()) {
      std::cerr << "worst point was not recorded\n";
      return 1;
    }

    std::cout << "expansion quality passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_expansion_quality failed: " << exc.what() << "\n";
    return 1;
  }
}

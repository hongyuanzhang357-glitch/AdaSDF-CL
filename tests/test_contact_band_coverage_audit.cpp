#include <adasdf/adasdf.h>

#include <iostream>

namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::AdaptiveSDFBlock makeBlock() {
  adasdf::AdaptiveSDFBlock block;
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.spacing = {0.1, 0.1, 0.1};
  block.phi.assign(27, 0.01);
  block.phi[gridIndex(1, 1, 1, 3, 3)] = 0.0;
  return block;
}

}  // namespace

int main() {
  try {
    adasdf::ContactBandSamplingOptions options;
    options.coverage_audit = true;
    options.coverage_samples_per_axis = 3;
    options.contact_band_width = 5e-4;

    adasdf::ContactBandMask covered;
    covered.nx = 3;
    covered.ny = 3;
    covered.nz = 3;
    covered.contact_band_node.assign(27, 0);
    covered.exact_required.assign(27, 0);
    covered.halo_node.assign(27, 0);
    covered.exact_required[gridIndex(1, 1, 1, 3, 3)] = 1;

    const adasdf::AdaptiveSDFBlock block = makeBlock();
    adasdf::ContactBandQualityMetrics pass =
        adasdf::ContactBandQualityAudit::auditBlock(
            block, block, covered, options);
    if (!pass.coverage_passed ||
        pass.contact_band_coverage_check_count != 1 ||
        pass.missed_contact_band_point_count != 0 ||
        !pass.contact_band_quality_passed) {
      std::cerr << "coverage audit should pass for covered contact point\n";
      return 1;
    }

    covered.exact_required[gridIndex(1, 1, 1, 3, 3)] = 0;
    adasdf::ContactBandQualityMetrics fail =
        adasdf::ContactBandQualityAudit::auditBlock(
            block, block, covered, options);
    if (fail.coverage_passed ||
        fail.missed_contact_band_point_count != 1 ||
        fail.missed_contact_band_cell_count != 1 ||
        fail.contact_band_quality_passed) {
      std::cerr << "coverage audit should fail for missed contact point\n";
      return 1;
    }

    std::cout << "contact-band coverage audit passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_band_coverage_audit failed: " << exc.what()
              << "\n";
    return 1;
  }
}

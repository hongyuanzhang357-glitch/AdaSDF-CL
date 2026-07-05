#include <adasdf/adasdf.h>

#include <iostream>

namespace {

std::size_t index3(int i, int j, int k, int n) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(n) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(n) * static_cast<std::size_t>(k));
}

adasdf::AdaptiveSDFBlock makeBlock(double sign) {
  adasdf::AdaptiveSDFBlock block;
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.spacing = {1.0, 1.0, 1.0};
  block.phi.assign(27, 0.0);
  for (int k = 0; k < 3; ++k) {
    for (int j = 0; j < 3; ++j) {
      for (int i = 0; i < 3; ++i) {
        block.phi[index3(i, j, k, 3)] = sign * (static_cast<double>(i) - 1.0);
      }
    }
  }
  return block;
}

}  // namespace

int main() {
  adasdf::ContactBandMask mask;
  mask.nx = 3;
  mask.ny = 3;
  mask.nz = 3;
  mask.contact_band_node.assign(27, 0);
  mask.exact_required.assign(27, 1);
  mask.halo_node.assign(27, 0);
  mask.contact_band_node[index3(1, 1, 1, 3)] = 1;
  adasdf::ContactBandSamplingOptions options;
  options.normal_audit = true;
  const auto exact = makeBlock(1.0);
  auto metrics =
      adasdf::ContactBandQualityAudit::auditBlock(exact, exact, mask, options);
  if (!metrics.contact_band_quality_passed ||
      metrics.contact_band_max_abs_error != 0.0 ||
      metrics.max_normal_angle_error_deg != 0.0) {
    std::cerr << "identical field should pass with zero error\n";
    return 1;
  }
  auto flipped = exact;
  flipped.phi[index3(1, 1, 1, 3)] = -1.0;
  metrics =
      adasdf::ContactBandQualityAudit::auditBlock(flipped, exact, mask, options);
  if (metrics.contact_band_quality_passed ||
      metrics.contact_band_sign_mismatch_count == 0) {
    std::cerr << "contact-band sign mismatch should fail quality\n";
    return 1;
  }
  std::cout << "contact band quality audit passed\n";
  return 0;
}

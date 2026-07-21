#include "adasdf/narrowband/BrickTensorFillPolicy.h"

#include <cmath>

namespace adasdf {

BrickTensorFillDecision BrickTensorFillPolicy::evaluate(
    const Vector3& point,
    const BVHSDFSampler& sampler,
    const NarrowBandBrickBuildOptions& options) {
  BrickTensorFillDecision decision;
  const BVHSDFSampleResult sample = sampler.sample(point);
  decision.phi =
      sample.success && std::isfinite(sample.phi) ? sample.phi : 0.0;
  if (options.tensor_fill == NarrowBandTensorFillMode::ExactAll) {
    decision.exact_source = true;
    decision.interpolated_fill = false;
    return decision;
  }
  const bool contact =
      std::abs(decision.phi) <= options.sampling_contact_band_width;
  decision.exact_source = contact;
  decision.interpolated_fill = !contact;
  return decision;
}

}  // namespace adasdf

#include "adasdf/narrowband/SamplingDemandEstimator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

Vector3 pointInBox(const AABB& box, double u, double v, double w) {
  return {
      box.min.x + (box.max.x - box.min.x) * u,
      box.min.y + (box.max.y - box.min.y) * v,
      box.min.z + (box.max.z - box.min.z) * w};
}

double diagonal(const AABB& box) {
  const Vector3 d = box.max - box.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

}  // namespace

SamplingDemandEstimate SamplingDemandEstimator::estimate(
    const AABB& bounds,
    int level,
    const BVHSDFSampler& sampler,
    const NarrowBandBrickBuildOptions& options) {
  SamplingDemandEstimate out;
  const double cell_diag = diagonal(bounds);
  out.target_sample_spacing = cell_diag;

  std::array<Vector3, 9> probes = {{
      pointInBox(bounds, 0.5, 0.5, 0.5),
      pointInBox(bounds, 0.0, 0.0, 0.0),
      pointInBox(bounds, 1.0, 0.0, 0.0),
      pointInBox(bounds, 0.0, 1.0, 0.0),
      pointInBox(bounds, 1.0, 1.0, 0.0),
      pointInBox(bounds, 0.0, 0.0, 1.0),
      pointInBox(bounds, 1.0, 0.0, 1.0),
      pointInBox(bounds, 0.0, 1.0, 1.0),
      pointInBox(bounds, 1.0, 1.0, 1.0)}};

  double min_abs_phi = std::numeric_limits<double>::infinity();
  bool has_pos = false;
  bool has_neg = false;
  for (const Vector3& p : probes) {
    const BVHSDFSampleResult sample = sampler.sample(p);
    if (!sample.success || !std::isfinite(sample.phi)) {
      continue;
    }
    min_abs_phi = std::min(min_abs_phi, std::abs(sample.phi));
    has_pos = has_pos || sample.phi > 0.0;
    has_neg = has_neg || sample.phi < 0.0;
  }
  if (!std::isfinite(min_abs_phi)) {
    min_abs_phi = cell_diag;
  }
  out.min_abs_phi = min_abs_phi;
  const double tolerance =
      options.sampling_contact_band_width + std::max(0.0, cell_diag * 0.10);
  out.near_zero_surface = min_abs_phi <= tolerance || (has_pos && has_neg);
  out.zero_crossing_risk = has_pos && has_neg;
  out.contact_band =
      min_abs_phi <= options.sampling_contact_band_width || (has_pos && has_neg);
  out.far_field = !out.contact_band;
  out.exact_required = out.contact_band || out.near_zero_surface;
  out.should_refine =
      level < options.max_sampling_level &&
      (level < 2 ||
       (options.sampling_refine_contact_band && out.near_zero_surface) ||
       (options.sampling_refine_zero_crossing && out.zero_crossing_risk) ||
       (options.sampling_refine_curvature_hint &&
        options.sampling_curvature_aware) ||
       (options.sampling_refine_small_gap_hint &&
        options.sampling_small_gap_aware));
  out.estimated_exact_sample_count = out.exact_required ? 8 : 0;
  out.estimated_interpolated_sample_count = out.exact_required ? 0 : 8;
  return out;
}

}  // namespace adasdf

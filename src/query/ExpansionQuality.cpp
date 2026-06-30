#include "adasdf/query/ExpansionQuality.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace adasdf {
namespace {

bool finitePoint(const Vector3& p) {
  return p.allFinite();
}

AABB qualityDomain(const SDFModel& direct_model, const ExpandedSDF& expanded) {
  AABB domain = expanded.boundingBox();
  if (!domain.valid) {
    domain = direct_model.boundingBox();
  }
  if (!domain.valid || !finitePoint(domain.min) || !finitePoint(domain.max) ||
      !(domain.min.x < domain.max.x) || !(domain.min.y < domain.max.y) ||
      !(domain.min.z < domain.max.z)) {
    throw std::runtime_error("ExpansionQuality requires a finite sample domain.");
  }
  return domain;
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

Vector3 volumeSample(const AABB& domain, std::mt19937& rng) {
  std::uniform_real_distribution<double> unit(0.0, 1.0);
  return {
      lerp(domain.min.x, domain.max.x, unit(rng)),
      lerp(domain.min.y, domain.max.y, unit(rng)),
      lerp(domain.min.z, domain.max.z, unit(rng))};
}

Vector3 nearBoxSurfaceSample(
    const AABB& model_bounds,
    const AABB& domain,
    double band,
    std::mt19937& rng) {
  std::uniform_real_distribution<double> unit(0.0, 1.0);
  std::uniform_real_distribution<double> offset(-band, band);
  std::uniform_int_distribution<int> face(0, 5);

  Vector3 p = volumeSample(model_bounds.valid ? model_bounds : domain, rng);
  const int f = face(rng);
  if (f == 0) {
    p.x = model_bounds.min.x + offset(rng);
  } else if (f == 1) {
    p.x = model_bounds.max.x + offset(rng);
  } else if (f == 2) {
    p.y = model_bounds.min.y + offset(rng);
  } else if (f == 3) {
    p.y = model_bounds.max.y + offset(rng);
  } else if (f == 4) {
    p.z = model_bounds.min.z + offset(rng);
  } else {
    p.z = model_bounds.max.z + offset(rng);
  }
  p.x = std::max(domain.min.x, std::min(domain.max.x, p.x));
  p.y = std::max(domain.min.y, std::min(domain.max.y, p.y));
  p.z = std::max(domain.min.z, std::min(domain.max.z, p.z));
  (void)unit;
  return p;
}

std::vector<Vector3> makeSamples(
    const SDFModel& direct_model,
    const ExpandedSDF& expanded,
    const ExpansionQualityOptions& options) {
  if (options.num_samples < 1) {
    throw std::runtime_error("ExpansionQuality num_samples must be positive.");
  }
  if (!options.include_volume_samples && !options.include_near_surface_samples) {
    throw std::runtime_error(
        "ExpansionQuality requires at least one sample family.");
  }

  const AABB domain = qualityDomain(direct_model, expanded);
  const AABB model_bounds = direct_model.boundingBox();
  std::mt19937 rng(options.seed);
  std::vector<Vector3> points;
  points.reserve(static_cast<std::size_t>(options.num_samples));
  for (int i = 0; i < options.num_samples; ++i) {
    const bool use_near =
        options.include_near_surface_samples &&
        (!options.include_volume_samples || (i % 2 == 1));
    if (use_near && model_bounds.valid &&
        options.near_surface_band > 0.0) {
      points.push_back(nearBoxSurfaceSample(
          model_bounds,
          domain,
          options.near_surface_band,
          rng));
    } else {
      points.push_back(volumeSample(domain, rng));
    }
  }
  return points;
}

double percentile95(std::vector<double> values) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const double rank = 0.95 * static_cast<double>(values.size() - 1);
  const auto index = static_cast<std::size_t>(std::ceil(rank));
  return values[std::min(index, values.size() - 1)];
}

std::string directBackendName(const SDFModel& model) {
  if (!model.metadata().query_backend.empty()) {
    return model.metadata().query_backend;
  }
  if (model.nativeHandle()) {
    return model.nativeHandle()->backendName();
  }
  return "direct";
}

}  // namespace

SDFSignClass classifySDFSign(double phi, double sign_epsilon) {
  const double eps = std::max(0.0, sign_epsilon);
  if (phi < -eps) {
    return SDFSignClass::Inside;
  }
  if (phi > eps) {
    return SDFSignClass::Outside;
  }
  return SDFSignClass::Ambiguous;
}

bool isStrictSignMismatch(
    double direct_phi,
    double expanded_phi,
    double sign_epsilon) {
  const SDFSignClass direct_sign =
      classifySDFSign(direct_phi, sign_epsilon);
  const SDFSignClass expanded_sign =
      classifySDFSign(expanded_phi, sign_epsilon);
  return (direct_sign == SDFSignClass::Inside &&
          expanded_sign == SDFSignClass::Outside) ||
         (direct_sign == SDFSignClass::Outside &&
          expanded_sign == SDFSignClass::Inside);
}

const char* toString(SDFSignClass sign_class) {
  switch (sign_class) {
    case SDFSignClass::Inside:
      return "inside";
    case SDFSignClass::Outside:
      return "outside";
    case SDFSignClass::Ambiguous:
      return "ambiguous";
  }
  return "unknown";
}

ExpansionQualityReport ExpansionQuality::compareAgainstDirect(
    const SDFModel& direct_model,
    const ExpandedSDF& expanded,
    const ExpansionQualityOptions& options) {
  if (!direct_model.isValid() || !direct_model.queryBackendAvailable()) {
    throw std::runtime_error(
        "ExpansionQuality requires a direct model with query support.");
  }
  if (!expanded.isValid()) {
    throw std::runtime_error("ExpansionQuality requires a valid ExpandedSDF.");
  }

  ExpansionQualityReport report;
  report.direct_backend = directBackendName(direct_model);
  report.expanded_backend = toString(expanded.layout());

  const std::vector<Vector3> points = makeSamples(direct_model, expanded, options);
  report.num_samples = static_cast<int>(points.size());

  std::vector<double> abs_errors;
  abs_errors.reserve(points.size());
  double sum_abs = 0.0;
  double sum_sq = 0.0;

  for (std::size_t i = 0; i < points.size(); ++i) {
    const Vector3& point = points[i];
    const double direct_phi = direct_model.sampleDistance(point);
    double expanded_phi = std::numeric_limits<double>::quiet_NaN();

    bool used_fallback = false;
    try {
      expanded_phi = expanded.sampleDistance(point);
    } catch (const std::exception&) {
      used_fallback = true;
      ++report.fallback_count;
      if (expanded.queryPolicy().allow_direct_fallback_outside) {
        expanded_phi = direct_phi;
      }
    }

    if (!std::isfinite(direct_phi) || !std::isfinite(expanded_phi)) {
      continue;
    }

    const double abs_error = std::abs(direct_phi - expanded_phi);
    abs_errors.push_back(abs_error);
    ++report.num_finite_samples;
    sum_abs += abs_error;
    sum_sq += abs_error * abs_error;

    if (abs_error > report.max_abs_error) {
      report.max_abs_error = abs_error;
      report.worst_point_id = static_cast<int>(i);
      report.worst_point = point;
      report.worst_direct_phi = direct_phi;
      report.worst_expanded_phi = expanded_phi;
    }

    const SDFSignClass direct_sign =
        classifySDFSign(direct_phi, options.sign_epsilon);
    const SDFSignClass expanded_sign =
        classifySDFSign(expanded_phi, options.sign_epsilon);
    if (direct_sign == SDFSignClass::Ambiguous ||
        expanded_sign == SDFSignClass::Ambiguous) {
      ++report.ambiguous_sign_count;
    } else if (isStrictSignMismatch(
                   direct_phi,
                   expanded_phi,
                   options.sign_epsilon)) {
      ++report.sign_mismatch_count;
    }

    const bool near_surface =
        std::abs(direct_phi) <= options.near_surface_band;
    if (near_surface) {
      ++report.near_surface_sample_count;
      if (isStrictSignMismatch(
              direct_phi,
              expanded_phi,
              options.sign_epsilon)) {
        ++report.near_surface_sign_mismatch_count;
      }
    }
    (void)used_fallback;
  }

  if (report.num_finite_samples > 0) {
    const double finite = static_cast<double>(report.num_finite_samples);
    report.mean_abs_error = sum_abs / finite;
    report.rms_error = std::sqrt(sum_sq / finite);
    report.p95_abs_error = percentile95(abs_errors);
    report.sign_mismatch_rate =
        static_cast<double>(report.sign_mismatch_count) / finite;
    report.ambiguous_sign_rate =
        static_cast<double>(report.ambiguous_sign_count) / finite;
  }
  if (report.near_surface_sample_count > 0) {
    report.near_surface_sign_mismatch_rate =
        static_cast<double>(report.near_surface_sign_mismatch_count) /
        static_cast<double>(report.near_surface_sample_count);
  }
  if (report.num_samples > 0) {
    report.fallback_rate =
        static_cast<double>(report.fallback_count) /
        static_cast<double>(report.num_samples);
  }
  return report;
}

}  // namespace adasdf

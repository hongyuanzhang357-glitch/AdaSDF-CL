#include "adasdf/audit/SDFAccuracyAudit.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>

#include "adasdf/compression/CompressedSDFBlock.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

double norm(const Vector3& v) {
  return std::sqrt(std::max(0.0, dot(v, v)));
}

Vector3 normalized(const Vector3& v) {
  const double len = norm(v);
  if (!(len > 0.0) || !std::isfinite(len)) {
    return Vector3::Zero();
  }
  return v / len;
}

bool containsPoint(const AABB& box, const Vector3& point) {
  const double eps = 1.0e-12;
  return box.valid && point.x >= box.min.x - eps &&
         point.x <= box.max.x + eps && point.y >= box.min.y - eps &&
         point.y <= box.max.y + eps && point.z >= box.min.z - eps &&
         point.z <= box.max.z + eps;
}

int signWithEps(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

std::string signPattern(const double values[8], double eps) {
  std::string out;
  out.reserve(8);
  for (int i = 0; i < 8; ++i) {
    const int s = signWithEps(values[i], eps);
    out.push_back(s < 0 ? '-' : (s > 0 ? '+' : '0'));
  }
  return out;
}

std::string offsetKey(double offset) {
  std::ostringstream out;
  out.setf(std::ios::fixed);
  out.precision(6);
  out << offset;
  return out.str();
}

std::string referencePhiBinKey(double phi) {
  const double value = std::abs(phi);
  if (value < 0.001) {
    return "[0,0.001)";
  }
  if (value < 0.002) {
    return "[0.001,0.002)";
  }
  if (value < 0.004) {
    return "[0.002,0.004)";
  }
  if (value <= 0.010) {
    return "[0.004,0.010]";
  }
  return ">0.010";
}

double percentile(std::vector<double> values, double q) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  if (values.size() == 1) {
    return values.front();
  }
  const double clamped = std::clamp(q, 0.0, 1.0);
  const double pos = clamped * static_cast<double>(values.size() - 1);
  const std::size_t lo = static_cast<std::size_t>(std::floor(pos));
  const std::size_t hi = std::min(lo + 1, values.size() - 1);
  const double t = pos - static_cast<double>(lo);
  return values[lo] + (values[hi] - values[lo]) * t;
}

std::size_t blockValueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
}

bool nearCellBoundary(double value) {
  const double f = std::abs(value - std::round(value));
  return f <= 1.0e-6;
}

template <typename BlockT, typename ValueFn>
void fillBlockInspection(
    const BlockT& block,
    const Vector3& point,
    const std::string& source,
    const ValueFn& value_fn,
    double sign_epsilon,
    SDFAccuracyAuditSample* item) {
  item->block_id = block.block_id;
  item->block_level = block.level;
  item->block_aabb = block.bounds;
  const double gx = axisCoord(point.x, block.origin.x, block.spacing.x, block.nx);
  const double gy = axisCoord(point.y, block.origin.y, block.spacing.y, block.ny);
  const double gz = axisCoord(point.z, block.origin.z, block.spacing.z, block.nz);
  item->local_i =
      std::max(0, std::min(block.nx - 2, static_cast<int>(std::floor(gx))));
  item->local_j =
      std::max(0, std::min(block.ny - 2, static_cast<int>(std::floor(gy))));
  item->local_k =
      std::max(0, std::min(block.nz - 2, static_cast<int>(std::floor(gz))));
  item->local_u = gx - static_cast<double>(item->local_i);
  item->local_v = gy - static_cast<double>(item->local_j);
  item->local_w = gz - static_cast<double>(item->local_k);
  item->corner_phi[0] = value_fn(item->local_i, item->local_j, item->local_k);
  item->corner_phi[1] = value_fn(item->local_i + 1, item->local_j, item->local_k);
  item->corner_phi[2] = value_fn(item->local_i, item->local_j + 1, item->local_k);
  item->corner_phi[3] = value_fn(item->local_i + 1, item->local_j + 1, item->local_k);
  item->corner_phi[4] = value_fn(item->local_i, item->local_j, item->local_k + 1);
  item->corner_phi[5] = value_fn(item->local_i + 1, item->local_j, item->local_k + 1);
  item->corner_phi[6] = value_fn(item->local_i, item->local_j + 1, item->local_k + 1);
  item->corner_phi[7] =
      value_fn(item->local_i + 1, item->local_j + 1, item->local_k + 1);
  item->corner_sign_pattern = signPattern(item->corner_phi, sign_epsilon);
  item->block_source = source;
  const bool boundary =
      item->local_i == 0 || item->local_j == 0 || item->local_k == 0 ||
      item->local_i >= block.nx - 2 || item->local_j >= block.ny - 2 ||
      item->local_k >= block.nz - 2 || nearCellBoundary(gx) ||
      nearCellBoundary(gy) || nearCellBoundary(gz);
  item->query_source = boundary ? "block_boundary" : "block_interior";
}

void applyProvenanceSource(
    const BlockProvenanceSet* provenance,
    SDFAccuracyAuditSample* item) {
  if (provenance == nullptr || item == nullptr || item->block_id < 0) {
    return;
  }
  const BlockProvenance* block = provenance->find(item->block_id);
  if (block == nullptr) {
    item->query_source = "unknown";
    return;
  }
  const bool fully_exact = block->logical_node_count > 0 &&
                           block->exact_node_count >= block->logical_node_count;
  const bool partial_exact = block->exact_node_count > 0 &&
                             block->predicted_node_count > 0;
  if (block->is_coverage_promoted) {
    item->block_source = fully_exact ? "coverage_promoted_block"
                         : partial_exact ? "coverage_promoted_partial_exact"
                                         : "coverage_promoted_block";
    item->query_source = "coverage_promoted_block";
    return;
  }
  if (block->is_contact_band_block) {
    item->block_source = fully_exact ? "contact_band_fully_exact"
                         : partial_exact ? "contact_band_partial_exact"
                                         : "contact_band_block";
    item->query_source = "contact_band_block";
    return;
  }
  if (block->is_far_field_block) {
    item->block_source =
        block->predicted_node_count > 0 ? "far_field_predicted_block"
                                        : "far_field_block";
    item->query_source = "far_field_block";
    return;
  }
  item->query_source = "unknown";
}

void inspectQuerySource(
    const SDFModel& sdf,
    const Vector3& point,
    double sign_epsilon,
    const BlockProvenanceSet* provenance,
    SDFAccuracyAuditSample* item) {
  if (item == nullptr) {
    return;
  }
  if (const auto* model =
          dynamic_cast<const AdaptiveBlockSDFModel*>(&sdf)) {
    const int block_index = model->findContainingBlock(point);
    if (block_index < 0) {
      item->query_source = "out_of_domain";
      return;
    }
    const AdaptiveSDFBlock& block =
        model->blockSet().blocks[static_cast<std::size_t>(block_index)];
    const auto value = [&](int i, int j, int k) {
      return block.phi[blockValueIndex(i, j, k, block.nx, block.ny)];
    };
    fillBlockInspection(
        block,
        point,
        "adaptive_dense",
        value,
        sign_epsilon,
        item);
    applyProvenanceSource(provenance, item);
    return;
  }
  if (const auto* model =
          dynamic_cast<const CompressedAdaptiveBlockSDFModel*>(&sdf)) {
    const int block_index = model->findContainingBlock(point);
    if (block_index < 0) {
      item->query_source = "out_of_domain";
      return;
    }
    const CompressedSDFBlock& block =
        model->compressedBlockSet().blocks[static_cast<std::size_t>(block_index)];
    const auto value = [&](int i, int j, int k) {
      return compressedBlockGridValue(block, i, j, k);
    };
    const std::string source =
        block.method == BlockCompressionMethod::DenseFallback
            ? "compressed_dense_fallback"
            : "compressed_svd";
    fillBlockInspection(
        block,
        point,
        source,
        value,
        sign_epsilon,
        item);
    applyProvenanceSource(provenance, item);
    return;
  }
  item->query_source = "non_block_model";
}

void accumulateDistanceStats(
    const std::vector<double>& values,
    double* max_value,
    double* mean_value,
    double* rms_value,
    double* p50,
    double* p95,
    double* p99) {
  if (values.empty()) {
    *max_value = 0.0;
    *mean_value = 0.0;
    *rms_value = 0.0;
    *p50 = 0.0;
    *p95 = 0.0;
    *p99 = 0.0;
    return;
  }
  *max_value = *std::max_element(values.begin(), values.end());
  *mean_value =
      std::accumulate(values.begin(), values.end(), 0.0) /
      static_cast<double>(values.size());
  double sum_square = 0.0;
  for (double value : values) {
    sum_square += value * value;
  }
  *rms_value = std::sqrt(sum_square / static_cast<double>(values.size()));
  *p50 = percentile(values, 0.50);
  *p95 = percentile(values, 0.95);
  *p99 = percentile(values, 0.99);
}

double autoNormalEps(const TriangleMesh& mesh) {
  const double diag = mesh.diagonalLength();
  if (std::isfinite(diag) && diag > 0.0) {
    return std::max(diag * 1.0e-6, 1.0e-6);
  }
  return 1.0e-6;
}

BVHSDFSampleResult timedReferenceSample(
    const BVHSDFSampler& sampler,
    const Vector3& point,
    SDFAccuracyAuditResult* result) {
  const auto begin = Clock::now();
  BVHSDFSampleResult sample = sampler.sample(point);
  const auto end = Clock::now();
  result->exact_reference_time_ms += elapsedMs(begin, end);
  ++result->exact_reference_query_count;
  return sample;
}

Vector3 referenceNormalCentralDifference(
    const BVHSDFSampler& sampler,
    const Vector3& point,
    double eps,
    SDFAccuracyAuditResult* result) {
  const Vector3 dx{eps, 0.0, 0.0};
  const Vector3 dy{0.0, eps, 0.0};
  const Vector3 dz{0.0, 0.0, eps};
  const auto px = timedReferenceSample(sampler, point + dx, result);
  const auto mx = timedReferenceSample(sampler, point - dx, result);
  const auto py = timedReferenceSample(sampler, point + dy, result);
  const auto my = timedReferenceSample(sampler, point - dy, result);
  const auto pz = timedReferenceSample(sampler, point + dz, result);
  const auto mz = timedReferenceSample(sampler, point - dz, result);
  if (!px.success || !mx.success || !py.success || !my.success ||
      !pz.success || !mz.success) {
    return Vector3::Zero();
  }
  return normalized({(px.phi - mx.phi) / (2.0 * eps),
                     (py.phi - my.phi) / (2.0 * eps),
                     (pz.phi - mz.phi) / (2.0 * eps)});
}

double angleDeg(const Vector3& a, const Vector3& b) {
  const Vector3 na = normalized(a);
  const Vector3 nb = normalized(b);
  if (norm(na) <= 0.0 || norm(nb) <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  const double cosine = std::clamp(dot(na, nb), -1.0, 1.0);
  return std::acos(cosine) * 180.0 / 3.14159265358979323846;
}

struct MutableBinStats {
  SDFAccuracyBinStats stats;
  std::vector<double> errors;
  std::vector<double> normal_angles;
};

void addToBin(
    std::map<std::string, MutableBinStats>* bins,
    const std::string& key,
    const std::string& label,
    const SDFAccuracyAuditSample& sample) {
  MutableBinStats& bin = (*bins)[key];
  bin.stats.key = key;
  bin.stats.label = label;
  ++bin.stats.sample_count;
  if (sample.sign_mismatch) {
    ++bin.stats.sign_mismatch_count;
  }
  if (sample.false_inside) {
    ++bin.stats.false_inside_count;
  }
  if (sample.false_outside) {
    ++bin.stats.false_outside_count;
  }
  if (sample.near_surface && !sample.query_failed) {
    bin.errors.push_back(sample.abs_error);
  }
  if (sample.normal_checked) {
    bin.normal_angles.push_back(sample.normal_angle_error_deg);
  }
}

std::vector<SDFAccuracyBinStats> finalizeBins(
    const std::map<std::string, MutableBinStats>& bins) {
  std::vector<SDFAccuracyBinStats> out;
  out.reserve(bins.size());
  for (const auto& item : bins) {
    SDFAccuracyBinStats stats = item.second.stats;
    stats.sign_mismatch_rate =
        stats.sample_count == 0
            ? 0.0
            : static_cast<double>(stats.sign_mismatch_count) /
                  static_cast<double>(stats.sample_count);
    stats.p95_abs_error = percentile(item.second.errors, 0.95);
    stats.p95_normal_angle_error_deg =
        percentile(item.second.normal_angles, 0.95);
    out.push_back(stats);
  }
  return out;
}

}  // namespace

SDFAccuracyAuditResult SDFAccuracyAudit::run(
    const SDFModel& sdf,
    const TriangleMesh& mesh,
    const NearSurfaceSampleSet& samples,
    const SDFAccuracyAuditOptions& options) {
  SDFAccuracyAuditResult result;
  result.case_id = options.case_id;
  result.input_stl = options.input_stl;
  result.input_sdf = options.input_sdf;
  result.sample_count_total = samples.samples.size();
  result.normal_audit_enabled = options.normal_audit;
  result.near_surface_p95_abs_error_limit =
      options.thresholds.near_surface_p95_abs_error_limit;
  result.near_surface_max_abs_error_limit =
      options.thresholds.near_surface_max_abs_error_limit;
  result.p95_normal_angle_error_deg_limit =
      options.thresholds.p95_normal_angle_error_deg_limit;
  result.samples.reserve(samples.samples.size());

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.fallback_to_bruteforce_sign = true;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options);

  const double normal_eps =
      options.normal_eps > 0.0 ? options.normal_eps : autoNormalEps(mesh);
  std::vector<double> near_errors;
  std::vector<double> normal_angles;
  near_errors.reserve(samples.samples.size());
  normal_angles.reserve(samples.samples.size());

  const AABB bounds = sdf.boundingBox();
  for (std::size_t i = 0; i < samples.samples.size(); ++i) {
    const NearSurfaceSample& source = samples.samples[i];
    SDFAccuracyAuditSample item;
    item.sample_id = i;
    item.surface_sample_id = source.surface_sample_id;
    item.triangle_index = source.triangle_index;
    item.offset = source.offset;
    item.point = source.point;
    item.out_of_domain = bounds.valid && !containsPoint(bounds, source.point);
    inspectQuerySource(
        sdf,
        source.point,
        options.sign_epsilon,
        options.use_provenance ? options.block_provenance : nullptr,
        &item);

    const BVHSDFSampleResult reference =
        timedReferenceSample(sampler, source.point, &result);
    if (!reference.success || !std::isfinite(reference.phi)) {
      item.query_failed = true;
      ++result.query_failed_count;
      result.samples.push_back(item);
      continue;
    }
    item.nearest_triangle_id = reference.nearest.triangle_index;
    item.reference_phi = reference.phi;
    if (options.reference_sign_mode == "normal-offset" &&
        std::abs(source.offset) > options.sign_epsilon) {
      const double sign = source.offset < 0.0 ? -1.0 : 1.0;
      item.reference_phi = sign * std::abs(reference.phi);
    }
    item.near_surface = std::abs(reference.phi) <= options.near_band;

    const auto sdf_begin = Clock::now();
    try {
      item.sdf_phi = sdf.sampleDistance(source.point);
      ++result.sdf_query_count;
    } catch (...) {
      item.query_failed = true;
    }
    const auto sdf_end = Clock::now();
    result.sdf_query_time_ms += elapsedMs(sdf_begin, sdf_end);
    if (item.query_failed || !std::isfinite(item.sdf_phi)) {
      item.query_failed = true;
      ++result.query_failed_count;
      result.samples.push_back(item);
      continue;
    }

    item.abs_error = std::abs(item.sdf_phi - item.reference_phi);
    if (item.out_of_domain) {
      ++result.out_of_domain_count;
    }
    if (item.near_surface) {
      ++result.near_surface_sample_count;
      near_errors.push_back(item.abs_error);
    }

    const int ref_sign = signWithEps(item.reference_phi, options.sign_epsilon);
    const int sdf_sign = signWithEps(item.sdf_phi, options.sign_epsilon);
    item.reference_sign = ref_sign;
    item.sdf_sign = sdf_sign;
    const bool skip_zero_offset =
        options.exclude_zero_offset_sign &&
        std::abs(source.offset) <= options.sign_epsilon;
    item.sign_mismatch =
        !skip_zero_offset &&
        ref_sign != 0 && sdf_sign != 0 && ref_sign != sdf_sign;
    item.false_inside = !skip_zero_offset && ref_sign > 0 && sdf_sign < 0;
    item.false_outside = !skip_zero_offset && ref_sign < 0 && sdf_sign > 0;
    if (item.sign_mismatch) {
      ++result.sign_mismatch_count;
      if (item.near_surface) {
        ++result.near_surface_sign_mismatch_count;
      }
    }
    if (item.false_inside) {
      ++result.false_inside_count;
    }
    if (item.false_outside) {
      ++result.false_outside_count;
    }

    if (options.normal_audit && item.near_surface) {
      const Vector3 reference_normal = referenceNormalCentralDifference(
          sampler,
          source.point,
          normal_eps,
          &result);
      const auto normal_begin = Clock::now();
      Vector3 sdf_normal = Vector3::Zero();
      bool normal_ok = false;
      try {
        sdf_normal = sdf.sampleNormal(source.point);
        normal_ok = sdf_normal.allFinite();
      } catch (...) {
        normal_ok = false;
      }
      const auto normal_end = Clock::now();
      result.sdf_normal_time_ms += elapsedMs(normal_begin, normal_end);
      if (normal_ok && norm(reference_normal) > 0.0 && norm(sdf_normal) > 0.0) {
        item.normal_checked = true;
        item.normal_angle_error_deg = angleDeg(sdf_normal, reference_normal);
        item.normal_flip = dot(normalized(sdf_normal), reference_normal) < 0.0;
        ++result.normal_check_count;
        normal_angles.push_back(item.normal_angle_error_deg);
        if (item.normal_flip) {
          ++result.normal_flip_count;
          if (item.near_surface) {
            ++result.near_surface_normal_flip_count;
          }
        }
      }
    }

    result.samples.push_back(item);
  }

  accumulateDistanceStats(
      near_errors,
      &result.max_abs_error,
      &result.mean_abs_error,
      &result.rms_abs_error,
      &result.p50_abs_error,
      &result.p95_abs_error,
      &result.p99_abs_error);
  if (!normal_angles.empty()) {
    result.mean_normal_angle_error_deg =
        std::accumulate(normal_angles.begin(), normal_angles.end(), 0.0) /
        static_cast<double>(normal_angles.size());
    result.p95_normal_angle_error_deg = percentile(normal_angles, 0.95);
    result.max_normal_angle_error_deg =
        *std::max_element(normal_angles.begin(), normal_angles.end());
  }

  const std::size_t sign_denominator =
      result.sample_count_total > result.query_failed_count
          ? result.sample_count_total - result.query_failed_count
          : 0;
  result.sign_mismatch_rate =
      sign_denominator == 0
          ? 0.0
          : static_cast<double>(result.sign_mismatch_count) /
                static_cast<double>(sign_denominator);
  result.ns_per_sdf_query =
      result.sdf_query_count == 0
          ? 0.0
          : result.sdf_query_time_ms * 1.0e6 /
                static_cast<double>(result.sdf_query_count);
  result.ns_per_reference_query =
      result.exact_reference_query_count == 0
          ? 0.0
          : result.exact_reference_time_ms * 1.0e6 /
                static_cast<double>(result.exact_reference_query_count);

  result.near_surface_quality_passed =
      result.near_surface_sample_count > 0 && result.query_failed_count == 0 &&
      result.p95_abs_error <=
          options.thresholds.near_surface_p95_abs_error_limit &&
      result.max_abs_error <=
          options.thresholds.near_surface_max_abs_error_limit;
  result.sign_quality_passed =
      !options.thresholds.require_zero_sign_mismatch ||
      result.sign_mismatch_count == 0;
  result.normal_quality_passed =
      !options.normal_audit ||
      (result.normal_check_count > 0 &&
       result.p95_normal_angle_error_deg <=
           options.thresholds.p95_normal_angle_error_deg_limit &&
       (!options.thresholds.require_zero_near_surface_normal_flip ||
        result.near_surface_normal_flip_count == 0));
  result.full_quality_passed = result.near_surface_quality_passed &&
                               result.sign_quality_passed &&
                               result.normal_quality_passed;
  if (options.cluster_sign_errors || options.offset_bin_report ||
      options.block_source_report || options.query_source_report) {
    std::map<std::string, MutableBinStats> offset_bins;
    std::map<std::string, MutableBinStats> phi_bins;
    std::map<std::string, MutableBinStats> level_bins;
    std::map<std::string, MutableBinStats> source_bins;
    std::map<std::string, MutableBinStats> query_bins;
    for (const SDFAccuracyAuditSample& sample : result.samples) {
      addToBin(
          &offset_bins,
          offsetKey(sample.offset),
          offsetKey(sample.offset),
          sample);
      const std::string phi_key = referencePhiBinKey(sample.reference_phi);
      addToBin(&phi_bins, phi_key, phi_key, sample);
      const std::string level_key =
          sample.block_level >= 0 ? std::to_string(sample.block_level)
                                  : "unknown";
      addToBin(&level_bins, level_key, level_key, sample);
      addToBin(&source_bins, sample.block_source, sample.block_source, sample);
      addToBin(&query_bins, sample.query_source, sample.query_source, sample);
    }
    result.offset_bins = finalizeBins(offset_bins);
    result.reference_phi_bins = finalizeBins(phi_bins);
    result.block_level_bins = finalizeBins(level_bins);
    result.block_source_bins = finalizeBins(source_bins);
    result.query_source_bins = finalizeBins(query_bins);
  }
  return result;
}

}  // namespace adasdf

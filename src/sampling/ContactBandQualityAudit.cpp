#include "adasdf/sampling/ContactBandQualityAudit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace adasdf {
namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

int signOf(double value) {
  if (value < 0.0) {
    return -1;
  }
  if (value > 0.0) {
    return 1;
  }
  return 0;
}

double percentile95(std::vector<double> values) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const std::size_t index =
      std::min(
          values.size() - 1,
          static_cast<std::size_t>(
              std::ceil(0.95 * static_cast<double>(values.size()))) -
              1);
  return values[index];
}

Vector3 gradientAt(const AdaptiveSDFBlock& block, int i, int j, int k) {
  const auto at = [&](int x, int y, int z) {
    x = std::max(0, std::min(block.nx - 1, x));
    y = std::max(0, std::min(block.ny - 1, y));
    z = std::max(0, std::min(block.nz - 1, z));
    return block.phi[gridIndex(x, y, z, block.nx, block.ny)];
  };
  const double dx = block.spacing.x == 0.0 ? 1.0 : block.spacing.x;
  const double dy = block.spacing.y == 0.0 ? 1.0 : block.spacing.y;
  const double dz = block.spacing.z == 0.0 ? 1.0 : block.spacing.z;
  return {
      (at(i + 1, j, k) - at(i - 1, j, k)) / (2.0 * dx),
      (at(i, j + 1, k) - at(i, j - 1, k)) / (2.0 * dy),
      (at(i, j, k + 1) - at(i, j, k - 1)) / (2.0 * dz)};
}

double norm(const Vector3& value) {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool exactStencilAvailable(
    const ContactBandMask& mask,
    int i,
    int j,
    int k) {
  const int nx = mask.nx;
  const int ny = mask.ny;
  const int nz = mask.nz;
  if (i <= 0 || j <= 0 || k <= 0 ||
      i >= nx - 1 || j >= ny - 1 || k >= nz - 1) {
    return false;
  }
  const int offsets[6][3] = {
      {-1, 0, 0},
      {1, 0, 0},
      {0, -1, 0},
      {0, 1, 0},
      {0, 0, -1},
      {0, 0, 1}};
  for (const auto& offset : offsets) {
    const std::size_t index =
        gridIndex(i + offset[0], j + offset[1], k + offset[2], nx, ny);
    if (mask.exact_required[index] == 0) {
      return false;
    }
  }
  return true;
}

int sampledIndex(int sample, int sample_count, int n) {
  if (n <= 1 || sample_count <= 1) {
    return 0;
  }
  const double t =
      static_cast<double>(sample) / static_cast<double>(sample_count - 1);
  return std::max(
      0,
      std::min(n - 1, static_cast<int>(std::round(t * (n - 1)))));
}

std::size_t cellIndexForNode(
    int i,
    int j,
    int k,
    int nx,
    int ny,
    int nz) {
  const int cx = std::max(0, std::min(nx - 2, i));
  const int cy = std::max(0, std::min(ny - 2, j));
  const int cz = std::max(0, std::min(nz - 2, k));
  return static_cast<std::size_t>(cx) +
         static_cast<std::size_t>(std::max(1, nx - 1)) *
             (static_cast<std::size_t>(cy) +
              static_cast<std::size_t>(std::max(1, ny - 1)) *
                  static_cast<std::size_t>(cz));
}

}  // namespace

ContactBandQualityMetrics ContactBandQualityAudit::auditBlock(
    const AdaptiveSDFBlock& candidate,
    const AdaptiveSDFBlock& exact_reference,
    const ContactBandMask& mask,
    const ContactBandSamplingOptions& options) {
  ContactBandQualityMetrics metrics;
  if (candidate.phi.size() != exact_reference.phi.size() ||
      candidate.phi.size() != mask.contact_band_node.size()) {
    return metrics;
  }

  std::vector<double> abs_errors;
  std::vector<double> normal_errors;
  std::vector<std::uint8_t> missed_cells(
      static_cast<std::size_t>(std::max(1, candidate.nx - 1)) *
          static_cast<std::size_t>(std::max(1, candidate.ny - 1)) *
          static_cast<std::size_t>(std::max(1, candidate.nz - 1)),
      0);
  double abs_sum = 0.0;
  double sq_sum = 0.0;
  for (int k = 0; k < candidate.nz; ++k) {
    for (int j = 0; j < candidate.ny; ++j) {
      for (int i = 0; i < candidate.nx; ++i) {
        const std::size_t index = gridIndex(i, j, k, candidate.nx, candidate.ny);
        if (mask.contact_band_node[index] == 0 &&
            (!options.global_quality_gate || mask.exact_required[index] == 0)) {
          continue;
        }
        const double error =
            std::abs(candidate.phi[index] - exact_reference.phi[index]);
        abs_errors.push_back(error);
        abs_sum += error;
        sq_sum += error * error;
        if (signOf(candidate.phi[index]) != signOf(exact_reference.phi[index])) {
          ++metrics.contact_band_sign_mismatch_count;
          ++metrics.near_surface_sign_mismatch_count;
        }

        if (options.normal_audit && exactStencilAvailable(mask, i, j, k)) {
          const Vector3 gc = gradientAt(candidate, i, j, k);
          const Vector3 gr = gradientAt(exact_reference, i, j, k);
          const double nc = norm(gc);
          const double nr = norm(gr);
          if (nc > 1e-12 && nr > 1e-12) {
            const double cosine =
                std::max(-1.0, std::min(1.0, dot(gc, gr) / (nc * nr)));
            const double angle =
                std::acos(cosine) * 180.0 / 3.14159265358979323846;
            normal_errors.push_back(angle);
            if (cosine < 0.0) {
              ++metrics.normal_flip_count;
              ++metrics.near_surface_normal_flip_count;
            }
          }
        }
      }
    }
  }

  if (options.coverage_audit) {
    const int sx =
        options.coverage_samples_per_axis > 0
            ? std::min(candidate.nx, options.coverage_samples_per_axis)
            : candidate.nx;
    const int sy =
        options.coverage_samples_per_axis > 0
            ? std::min(candidate.ny, options.coverage_samples_per_axis)
            : candidate.ny;
    const int sz =
        options.coverage_samples_per_axis > 0
            ? std::min(candidate.nz, options.coverage_samples_per_axis)
            : candidate.nz;
    const double band = std::max(0.0, options.contact_band_width);
    for (int kk = 0; kk < sz; ++kk) {
      const int k = sampledIndex(kk, sz, candidate.nz);
      for (int jj = 0; jj < sy; ++jj) {
        const int j = sampledIndex(jj, sy, candidate.ny);
        for (int ii = 0; ii < sx; ++ii) {
          const int i = sampledIndex(ii, sx, candidate.nx);
          const std::size_t index =
              gridIndex(i, j, k, candidate.nx, candidate.ny);
          if (std::abs(exact_reference.phi[index]) > band) {
            continue;
          }
          ++metrics.contact_band_coverage_check_count;
          if (index >= mask.exact_required.size() ||
              mask.exact_required[index] == 0) {
            ++metrics.missed_contact_band_point_count;
            const std::size_t cell =
                cellIndexForNode(i, j, k, candidate.nx, candidate.ny, candidate.nz);
            if (cell < missed_cells.size() && missed_cells[cell] == 0) {
              missed_cells[cell] = 1;
              ++metrics.missed_contact_band_cell_count;
            }
          }
        }
      }
    }
  }

  metrics.contact_band_check_count = abs_errors.size();
  if (!abs_errors.empty()) {
    metrics.contact_band_max_abs_error =
        *std::max_element(abs_errors.begin(), abs_errors.end());
    metrics.contact_band_mean_abs_error =
        abs_sum / static_cast<double>(abs_errors.size());
    metrics.contact_band_rms_error =
        std::sqrt(sq_sum / static_cast<double>(abs_errors.size()));
    metrics.contact_band_p95_error = percentile95(abs_errors);
  }
  if (!normal_errors.empty()) {
    double normal_sum = 0.0;
    for (const double value : normal_errors) {
      normal_sum += value;
    }
    metrics.mean_normal_angle_error_deg =
        normal_sum / static_cast<double>(normal_errors.size());
    metrics.p95_normal_angle_error_deg = percentile95(normal_errors);
    metrics.max_normal_angle_error_deg =
        *std::max_element(normal_errors.begin(), normal_errors.end());
  }
  finalize(&metrics, options);
  return metrics;
}

ContactBandQualityMetrics ContactBandQualityAudit::merge(
    const ContactBandQualityMetrics& lhs,
    const ContactBandQualityMetrics& rhs) {
  ContactBandQualityMetrics out;
  const double lc = static_cast<double>(lhs.contact_band_check_count);
  const double rc = static_cast<double>(rhs.contact_band_check_count);
  const double total = lc + rc;
  out.contact_band_check_count =
      lhs.contact_band_check_count + rhs.contact_band_check_count;
  out.contact_band_coverage_check_count =
      lhs.contact_band_coverage_check_count +
      rhs.contact_band_coverage_check_count;
  out.missed_contact_band_point_count =
      lhs.missed_contact_band_point_count +
      rhs.missed_contact_band_point_count;
  out.missed_contact_band_cell_count =
      lhs.missed_contact_band_cell_count +
      rhs.missed_contact_band_cell_count;
  out.coverage_passed = lhs.coverage_passed && rhs.coverage_passed;
  out.contact_band_max_abs_error =
      std::max(lhs.contact_band_max_abs_error, rhs.contact_band_max_abs_error);
  if (total > 0.0) {
    out.contact_band_mean_abs_error =
        (lhs.contact_band_mean_abs_error * lc +
         rhs.contact_band_mean_abs_error * rc) /
        total;
    out.contact_band_rms_error =
        std::sqrt(
            (lhs.contact_band_rms_error * lhs.contact_band_rms_error * lc +
             rhs.contact_band_rms_error * rhs.contact_band_rms_error * rc) /
            total);
  }
  out.contact_band_p95_error =
      std::max(lhs.contact_band_p95_error, rhs.contact_band_p95_error);
  out.contact_band_sign_mismatch_count =
      lhs.contact_band_sign_mismatch_count +
      rhs.contact_band_sign_mismatch_count;
  out.near_surface_sign_mismatch_count =
      lhs.near_surface_sign_mismatch_count +
      rhs.near_surface_sign_mismatch_count;
  if (total > 0.0) {
    out.mean_normal_angle_error_deg =
        (lhs.mean_normal_angle_error_deg * lc +
         rhs.mean_normal_angle_error_deg * rc) /
        total;
  }
  out.p95_normal_angle_error_deg =
      std::max(lhs.p95_normal_angle_error_deg, rhs.p95_normal_angle_error_deg);
  out.max_normal_angle_error_deg =
      std::max(lhs.max_normal_angle_error_deg, rhs.max_normal_angle_error_deg);
  out.normal_flip_count = lhs.normal_flip_count + rhs.normal_flip_count;
  out.near_surface_normal_flip_count =
      lhs.near_surface_normal_flip_count + rhs.near_surface_normal_flip_count;
  out.contact_band_quality_passed =
      lhs.contact_band_quality_passed && rhs.contact_band_quality_passed;
  return out;
}

void ContactBandQualityAudit::finalize(
    ContactBandQualityMetrics* metrics,
    const ContactBandSamplingOptions& options) {
  if (metrics == nullptr) {
    return;
  }
  metrics->contact_band_quality_passed =
      metrics->contact_band_p95_error <= options.contact_band_phi_error_limit &&
      metrics->contact_band_sign_mismatch_count == 0 &&
      metrics->near_surface_sign_mismatch_count == 0 &&
      metrics->p95_normal_angle_error_deg <=
          options.contact_band_normal_error_limit_deg &&
      metrics->near_surface_normal_flip_count == 0 &&
      metrics->coverage_passed;
  metrics->coverage_passed =
      !options.coverage_audit ||
      metrics->missed_contact_band_point_count == 0;
  metrics->contact_band_quality_passed =
      metrics->contact_band_quality_passed && metrics->coverage_passed;
}

}  // namespace adasdf

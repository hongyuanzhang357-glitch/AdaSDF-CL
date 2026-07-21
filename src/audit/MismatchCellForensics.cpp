#include "adasdf/audit/MismatchCellForensics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>

#include "adasdf/geometry/BoxTriangleDistance.h"
#include "adasdf/mesh/TriangleDistance.h"

namespace adasdf {
namespace {

struct PatternAccumulator {
  std::size_t sample_count = 0;
  std::size_t sign_mismatch_count = 0;
  std::size_t false_inside_count = 0;
  std::size_t false_outside_count = 0;
  double block_level_sum = 0.0;
  std::vector<double> abs_errors;
  std::vector<double> normal_angles;
  std::map<int, std::size_t> block_counts;
};

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
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

int signWithEps(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
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

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

double trilinear(const double c[8], double u, double v, double w) {
  const double c00 = lerp(c[0], c[1], u);
  const double c10 = lerp(c[2], c[3], u);
  const double c01 = lerp(c[4], c[5], u);
  const double c11 = lerp(c[6], c[7], u);
  const double c0 = lerp(c00, c10, v);
  const double c1 = lerp(c01, c11, v);
  return lerp(c0, c1, w);
}

bool containsPoint(const AABB& box, const Vector3& p, double eps = 1.0e-12) {
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

AABB makeCellAABB(const AdaptiveSDFBlock& block, int i, int j, int k) {
  AABB box;
  box.valid = true;
  box.min = {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
  box.max = {
      block.origin.x + static_cast<double>(i + 1) * block.spacing.x,
      block.origin.y + static_cast<double>(j + 1) * block.spacing.y,
      block.origin.z + static_cast<double>(k + 1) * block.spacing.z};
  return box;
}

AABB expandAABB(const AABB& box, double amount) {
  AABB out = box;
  out.min = {out.min.x - amount, out.min.y - amount, out.min.z - amount};
  out.max = {out.max.x + amount, out.max.y + amount, out.max.z + amount};
  return out;
}

AABB triangleAABB(const Vector3& a, const Vector3& b, const Vector3& c) {
  AABB out;
  out.valid = true;
  out.min = {
      std::min({a.x, b.x, c.x}),
      std::min({a.y, b.y, c.y}),
      std::min({a.z, b.z, c.z})};
  out.max = {
      std::max({a.x, b.x, c.x}),
      std::max({a.y, b.y, c.y}),
      std::max({a.z, b.z, c.z})};
  return out;
}

bool aabbOverlap(const AABB& a, const AABB& b) {
  return a.valid && b.valid &&
         a.min.x <= b.max.x && a.max.x >= b.min.x &&
         a.min.y <= b.max.y && a.max.y >= b.min.y &&
         a.min.z <= b.max.z && a.max.z >= b.min.z;
}

Vector3 closestPointOnTriangle(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c) {
  const Vector3 ab = b - a;
  const Vector3 ac = c - a;
  const Vector3 ap = p - a;
  const double d1 = dot(ab, ap);
  const double d2 = dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) {
    return a;
  }
  const Vector3 bp = p - b;
  const double d3 = dot(ab, bp);
  const double d4 = dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3) {
    return b;
  }
  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const double v = d1 / (d1 - d3);
    return a + ab * v;
  }
  const Vector3 cp = p - c;
  const double d5 = dot(ab, cp);
  const double d6 = dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6) {
    return c;
  }
  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
    const double w = d2 / (d2 - d6);
    return a + ac * w;
  }
  const double va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
    const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + (c - b) * w;
  }
  const double denom = 1.0 / (va + vb + vc);
  const double v = vb * denom;
  const double w = vc * denom;
  return a + ab * v + ac * w;
}

Vector3 cellPoint(const AABB& box, double u, double v, double w) {
  return {
      lerp(box.min.x, box.max.x, u),
      lerp(box.min.y, box.max.y, v),
      lerp(box.min.z, box.max.z, w)};
}

std::array<Vector3, 8> cellCorners(const AABB& box) {
  return {
      cellPoint(box, 0.0, 0.0, 0.0),
      cellPoint(box, 1.0, 0.0, 0.0),
      cellPoint(box, 0.0, 1.0, 0.0),
      cellPoint(box, 1.0, 1.0, 0.0),
      cellPoint(box, 0.0, 0.0, 1.0),
      cellPoint(box, 1.0, 0.0, 1.0),
      cellPoint(box, 0.0, 1.0, 1.0),
      cellPoint(box, 1.0, 1.0, 1.0)};
}

double distanceToCellBoundary(const AABB& box, const Vector3& p) {
  return std::min({
      std::abs(p.x - box.min.x),
      std::abs(box.max.x - p.x),
      std::abs(p.y - box.min.y),
      std::abs(box.max.y - p.y),
      std::abs(p.z - box.min.z),
      std::abs(box.max.z - p.z)});
}

std::string signPattern(const double values[8], double eps) {
  std::string out;
  out.reserve(8);
  for (int i = 0; i < 8; ++i) {
    const int s = signWithEps(values[i], eps);
    out.push_back(s > 0 ? '+' : (s < 0 ? '-' : '0'));
  }
  return out;
}

bool edgeCrossing(const std::array<int, 8>& signs) {
  static constexpr int edges[12][2] = {
      {0, 1}, {2, 3}, {4, 5}, {6, 7},
      {0, 2}, {1, 3}, {4, 6}, {5, 7},
      {0, 4}, {1, 5}, {2, 6}, {3, 7}};
  for (const auto& edge : edges) {
    if (signs[edge[0]] != 0 && signs[edge[1]] != 0 &&
        signs[edge[0]] != signs[edge[1]]) {
      return true;
    }
  }
  return false;
}

bool faceCrossing(const std::array<int, 8>& signs) {
  static constexpr int faces[6][4] = {
      {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 4, 5},
      {2, 3, 6, 7}, {0, 2, 4, 6}, {1, 3, 5, 7}};
  for (const auto& face : faces) {
    bool has_pos = false;
    bool has_neg = false;
    for (int idx : face) {
      has_pos = has_pos || signs[idx] > 0;
      has_neg = has_neg || signs[idx] < 0;
    }
    if (has_pos && has_neg) {
      return true;
    }
  }
  return false;
}

bool checkerboardLike(const std::array<int, 8>& signs) {
  static constexpr int parity[8] = {0, 1, 1, 0, 1, 0, 0, 1};
  int base = 0;
  for (int i = 0; i < 8; ++i) {
    if (signs[i] == 0) {
      return false;
    }
    const int expected = parity[i] == 0 ? signs[0] : -signs[0];
    if (signs[i] != expected) {
      return false;
    }
    base = signs[i];
  }
  return base != 0;
}

std::string cornerPatternCategory(
    const double values[8],
    double eps,
    std::size_t* positive_count,
    std::size_t* negative_count,
    std::size_t* zero_count,
    bool* checkerboard,
    bool* single_opposite,
    bool* edge,
    bool* face) {
  std::array<int, 8> signs{};
  std::size_t pos = 0;
  std::size_t neg = 0;
  std::size_t zero = 0;
  for (int i = 0; i < 8; ++i) {
    signs[i] = signWithEps(values[i], eps);
    if (signs[i] > 0) {
      ++pos;
    } else if (signs[i] < 0) {
      ++neg;
    } else {
      ++zero;
    }
  }
  if (positive_count != nullptr) {
    *positive_count = pos;
  }
  if (negative_count != nullptr) {
    *negative_count = neg;
  }
  if (zero_count != nullptr) {
    *zero_count = zero;
  }
  if (checkerboard != nullptr) {
    *checkerboard = checkerboardLike(signs);
  }
  if (single_opposite != nullptr) {
    *single_opposite = pos > 0 && neg > 0 && std::min(pos, neg) == 1;
  }
  if (edge != nullptr) {
    *edge = edgeCrossing(signs);
  }
  if (face != nullptr) {
    *face = faceCrossing(signs);
  }
  if (pos == 8) {
    return "corner_pattern_all_positive";
  }
  if (neg == 8) {
    return "corner_pattern_all_negative";
  }
  if (pos > 0 && neg > 0) {
    return "corner_pattern_mixed_sign";
  }
  return "corner_pattern_has_near_zero_corner";
}

double exactPhi(BVHSDFSampler* sampler, const Vector3& p) {
  const BVHSDFSampleResult sample = sampler->sample(p);
  return sample.success && std::isfinite(sample.phi) ? sample.phi : 0.0;
}

std::vector<double> stencil27(const AABB& box, BVHSDFSampler* sampler) {
  std::vector<double> values;
  values.reserve(27);
  for (double w : {0.0, 0.5, 1.0}) {
    for (double v : {0.0, 0.5, 1.0}) {
      for (double u : {0.0, 0.5, 1.0}) {
        values.push_back(exactPhi(sampler, cellPoint(box, u, v, w)));
      }
    }
  }
  return values;
}

const BlockProvenance* provenanceFor(
    const BlockProvenanceSet* provenance,
    int block_id) {
  return provenance != nullptr ? provenance->find(block_id) : nullptr;
}

bool isMixedLevelBoundary(
    const AdaptiveBlockSDFModel& model,
    const AdaptiveSDFBlock& block,
    const AABB& cell) {
  const double eps =
      0.25 * std::min({block.spacing.x, block.spacing.y, block.spacing.z});
  const Vector3 center = cellPoint(cell, 0.5, 0.5, 0.5);
  const std::array<Vector3, 6> probes = {{
      {cell.min.x - eps, center.y, center.z},
      {cell.max.x + eps, center.y, center.z},
      {center.x, cell.min.y - eps, center.z},
      {center.x, cell.max.y + eps, center.z},
      {center.x, center.y, cell.min.z - eps},
      {center.x, center.y, cell.max.z + eps}}};
  for (const Vector3& probe : probes) {
    const int index = model.findContainingBlock(probe);
    if (index < 0) {
      continue;
    }
    const AdaptiveSDFBlock& other =
        model.blockSet().blocks[static_cast<std::size_t>(index)];
    if (other.block_id != block.block_id && other.level != block.level) {
      return true;
    }
  }
  return false;
}

void addStats(
    std::map<std::string, PatternAccumulator>* bins,
    const std::string& key,
    const MismatchCellSampleDiagnostic& sample) {
  PatternAccumulator& bin = (*bins)[key];
  ++bin.sample_count;
  ++bin.sign_mismatch_count;
  if (sample.false_inside) {
    ++bin.false_inside_count;
  }
  if (sample.false_outside) {
    ++bin.false_outside_count;
  }
  bin.block_level_sum += static_cast<double>(sample.block_level);
  bin.abs_errors.push_back(sample.abs_error);
  ++bin.block_counts[sample.block_id];
}

std::vector<int> topBlocks(const std::map<int, std::size_t>& counts) {
  std::vector<std::pair<int, std::size_t>> items(
      counts.begin(),
      counts.end());
  std::sort(
      items.begin(),
      items.end(),
      [](const auto& a, const auto& b) {
        if (a.second != b.second) {
          return a.second > b.second;
        }
        return a.first < b.first;
      });
  std::vector<int> out;
  for (std::size_t i = 0; i < std::min<std::size_t>(items.size(), 8); ++i) {
    out.push_back(items[i].first);
  }
  return out;
}

std::vector<MismatchCellPatternStats> finalizeStats(
    const std::map<std::string, PatternAccumulator>& bins) {
  std::vector<MismatchCellPatternStats> out;
  out.reserve(bins.size());
  for (const auto& item : bins) {
    MismatchCellPatternStats stats;
    stats.key = item.first;
    stats.sample_count = item.second.sample_count;
    stats.sign_mismatch_count = item.second.sign_mismatch_count;
    stats.false_inside_count = item.second.false_inside_count;
    stats.false_outside_count = item.second.false_outside_count;
    stats.p95_abs_error = percentile(item.second.abs_errors, 0.95);
    stats.p95_normal_angle_error_deg =
        percentile(item.second.normal_angles, 0.95);
    stats.average_block_level =
        item.second.sample_count > 0
            ? item.second.block_level_sum /
                  static_cast<double>(item.second.sample_count)
            : 0.0;
    stats.top_block_ids = topBlocks(item.second.block_counts);
    out.push_back(std::move(stats));
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    if (a.sample_count != b.sample_count) {
      return a.sample_count > b.sample_count;
    }
    return a.key < b.key;
  });
  return out;
}

std::string majorityPattern(
    const std::map<std::string, std::size_t>& counts) {
  if (counts.empty()) {
    return "none";
  }
  return std::max_element(
             counts.begin(),
             counts.end(),
             [](const auto& a, const auto& b) {
               if (a.second != b.second) {
                 return a.second < b.second;
               }
               return a.first > b.first;
             })
      ->first;
}

}  // namespace

MismatchCellForensicsResult MismatchCellForensics::run(
    const AdaptiveBlockSDFModel& model,
    const TriangleMesh& mesh,
    const std::vector<SDFAccuracyAuditSample>& mismatch_samples,
    const MismatchCellForensicsOptions& options) {
  MismatchCellForensicsResult result;
  result.case_id = options.case_id;

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.fallback_to_bruteforce_sign = true;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options);

  std::vector<double> errors;
  std::map<std::string, PatternAccumulator> pattern_bins;
  std::map<std::string, PatternAccumulator> zero_bins;
  std::map<std::string, std::size_t> false_inside_patterns;
  std::map<std::string, std::size_t> false_outside_patterns;
  std::map<int, MismatchCellSubgridSummary> subgrid_summaries;
  for (int subgrid : options.local_subgrids) {
    subgrid_summaries[subgrid].subgrid = subgrid;
  }

  const std::size_t limit =
      options.max_cells == 0
          ? mismatch_samples.size()
          : std::min(options.max_cells, mismatch_samples.size());
  for (std::size_t sample_index = 0; sample_index < limit; ++sample_index) {
    const SDFAccuracyAuditSample& source = mismatch_samples[sample_index];
    if (!source.sign_mismatch) {
      continue;
    }
    MismatchCellSampleDiagnostic item;
    item.sample_id = source.sample_id;
    item.point = source.point;
    item.reference_phi = source.reference_phi;
    item.sdf_phi = source.sdf_phi;
    item.reference_sign = source.reference_sign;
    item.sdf_sign = source.sdf_sign;
    item.false_inside = source.false_inside;
    item.false_outside = source.false_outside;
    item.abs_error = source.abs_error;
    item.nearest_triangle_id = source.nearest_triangle_id;
    item.exact_bvh_phi = source.reference_phi;
    errors.push_back(item.abs_error);

    const int block_index = model.findContainingBlock(source.point);
    if (block_index < 0) {
      item.block_lookup_suspicious = true;
      item.zero_crossing_category = "block_lookup_suspicious";
      ++result.block_lookup_suspicious_count;
      result.samples.push_back(std::move(item));
      continue;
    }
    const AdaptiveSDFBlock& block =
        model.blockSet().blocks[static_cast<std::size_t>(block_index)];
    item.block_id = block.block_id;
    item.block_level = block.level;
    item.block_aabb = block.bounds;
    item.block_nx = block.nx;
    item.block_ny = block.ny;
    item.block_nz = block.nz;
    item.block_lookup_suspicious =
        source.block_id >= 0 && source.block_id != block.block_id;
    if (item.block_lookup_suspicious) {
      ++result.block_lookup_suspicious_count;
    }

    const double gx =
        std::clamp(
            (source.point.x - block.origin.x) / block.spacing.x,
            0.0,
            static_cast<double>(block.nx - 1));
    const double gy =
        std::clamp(
            (source.point.y - block.origin.y) / block.spacing.y,
            0.0,
            static_cast<double>(block.ny - 1));
    const double gz =
        std::clamp(
            (source.point.z - block.origin.z) / block.spacing.z,
            0.0,
            static_cast<double>(block.nz - 1));
    item.local_i =
        std::max(0, std::min(block.nx - 2, static_cast<int>(std::floor(gx))));
    item.local_j =
        std::max(0, std::min(block.ny - 2, static_cast<int>(std::floor(gy))));
    item.local_k =
        std::max(0, std::min(block.nz - 2, static_cast<int>(std::floor(gz))));
    item.local_u = gx - static_cast<double>(item.local_i);
    item.local_v = gy - static_cast<double>(item.local_j);
    item.local_w = gz - static_cast<double>(item.local_k);
    item.cell_aabb =
        makeCellAABB(block, item.local_i, item.local_j, item.local_k);
    item.distance_to_cell_boundary =
        distanceToCellBoundary(item.cell_aabb, source.point);
    item.block_boundary_cell =
        item.local_i == 0 || item.local_j == 0 || item.local_k == 0 ||
        item.local_i >= block.nx - 2 || item.local_j >= block.ny - 2 ||
        item.local_k >= block.nz - 2;
    if (item.block_boundary_cell) {
      ++result.block_boundary_mismatch_count;
    }
    item.mixed_level_boundary_cell =
        item.block_boundary_cell &&
        isMixedLevelBoundary(model, block, item.cell_aabb);
    if (item.mixed_level_boundary_cell) {
      ++result.mixed_level_boundary_mismatch_count;
    }

    const BlockProvenance* provenance =
        provenanceFor(options.provenance, item.block_id);
    item.provenance_exists = provenance != nullptr;
    if (provenance != nullptr) {
      item.contact_band_block = provenance->is_contact_band_block;
      item.coverage_promoted_block = provenance->is_coverage_promoted;
      item.far_field_block = provenance->is_far_field_block;
    }

    const auto block_phi = [&](int i, int j, int k) {
      return block.phi[valueIndex(i, j, k, block.nx, block.ny)];
    };
    item.corner_phi[0] = block_phi(item.local_i, item.local_j, item.local_k);
    item.corner_phi[1] = block_phi(item.local_i + 1, item.local_j, item.local_k);
    item.corner_phi[2] = block_phi(item.local_i, item.local_j + 1, item.local_k);
    item.corner_phi[3] =
        block_phi(item.local_i + 1, item.local_j + 1, item.local_k);
    item.corner_phi[4] = block_phi(item.local_i, item.local_j, item.local_k + 1);
    item.corner_phi[5] =
        block_phi(item.local_i + 1, item.local_j, item.local_k + 1);
    item.corner_phi[6] =
        block_phi(item.local_i, item.local_j + 1, item.local_k + 1);
    item.corner_phi[7] =
        block_phi(item.local_i + 1, item.local_j + 1, item.local_k + 1);
    item.corner_sign_pattern =
        signPattern(item.corner_phi, options.sign_epsilon);
    bool checkerboard = false;
    bool single_opposite = false;
    bool edge = false;
    bool face = false;
    item.corner_pattern_category = cornerPatternCategory(
        item.corner_phi,
        options.sign_epsilon,
        &item.positive_corner_count,
        &item.negative_corner_count,
        &item.near_zero_corner_count,
        &checkerboard,
        &single_opposite,
        &edge,
        &face);
    item.trilinear_phi =
        trilinear(item.corner_phi, item.local_u, item.local_v, item.local_w);
    if (item.corner_pattern_category == "corner_pattern_all_positive") {
      ++result.corner_pattern_all_positive_count;
    } else if (item.corner_pattern_category ==
               "corner_pattern_all_negative") {
      ++result.corner_pattern_all_negative_count;
    }
    if (item.positive_corner_count > 0 && item.negative_corner_count > 0) {
      ++result.corner_pattern_mixed_sign_count;
    }
    if (item.near_zero_corner_count > 0) {
      ++result.corner_pattern_has_near_zero_corner_count;
    }
    if (checkerboard) {
      ++result.corner_pattern_checkerboard_like_count;
    }
    if (single_opposite) {
      ++result.corner_pattern_single_opposite_corner_count;
    }
    if (edge) {
      ++result.corner_pattern_edge_crossing_count;
    }
    if (face) {
      ++result.corner_pattern_face_crossing_count;
    }

    if (item.false_inside) {
      ++false_inside_patterns[item.corner_pattern_category];
    }
    if (item.false_outside) {
      ++false_outside_patterns[item.corner_pattern_category];
    }

    const auto corners = cellCorners(item.cell_aabb);
    for (std::size_t i = 0; i < corners.size(); ++i) {
      item.exact_corner_phi[i] = exactPhi(&sampler, corners[i]);
    }
    item.exact_center_phi =
        exactPhi(&sampler, cellPoint(item.cell_aabb, 0.5, 0.5, 0.5));
    if (options.enable_27_point_stencil) {
      item.exact_stencil_phi = stencil27(item.cell_aabb, &sampler);
    }

    int triangle_index = item.nearest_triangle_id;
    if (triangle_index < 0) {
      const BVHSDFSampleResult nearest = sampler.sample(source.point);
      triangle_index = nearest.nearest.triangle_index;
      item.nearest_triangle_id = triangle_index;
    }
    if (triangle_index >= 0 &&
        triangle_index < static_cast<int>(mesh.triangles.size())) {
      const MeshTriangle& tri =
          mesh.triangles[static_cast<std::size_t>(triangle_index)];
      const Vector3 a = toVector3(mesh.vertices[static_cast<std::size_t>(tri.v0)]);
      const Vector3 b = toVector3(mesh.vertices[static_cast<std::size_t>(tri.v1)]);
      const Vector3 c = toVector3(mesh.vertices[static_cast<std::size_t>(tri.v2)]);
      item.nearest_point_on_triangle =
          closestPointOnTriangle(source.point, a, b, c);
      item.nearest_triangle_normal = normalized(cross(b - a, c - a));
      const AABB tri_box = triangleAABB(a, b, c);
      item.triangle_cell_aabb_overlap = aabbOverlap(tri_box, item.cell_aabb);
      const AABB expanded =
          expandAABB(item.cell_aabb, std::max(options.near_band, 1.0e-12));
      item.expanded_triangle_cell_aabb_overlap =
          aabbOverlap(tri_box, expanded);
      const BoxTriangleDistanceResult box_tri =
          BoxTriangleDistance::estimate(item.cell_aabb, a, b, c);
      item.box_triangle_approx_distance = box_tri.approximate_distance;
      item.nearest_surface_point_inside_cell =
          containsPoint(item.cell_aabb, item.nearest_point_on_triangle);
    }
    item.reference_sign_suspicious =
        item.nearest_triangle_id < 0 ||
        (!item.triangle_cell_aabb_overlap &&
         !item.expanded_triangle_cell_aabb_overlap &&
         !item.nearest_surface_point_inside_cell &&
         std::abs(item.reference_phi) > options.near_band);
    if (item.reference_sign_suspicious) {
      ++result.reference_sign_suspicious_count;
    }

    CellZeroCrossingInput zero_input;
    zero_input.triangle_aabb_overlap = item.triangle_cell_aabb_overlap;
    zero_input.expanded_triangle_aabb_overlap =
        item.expanded_triangle_cell_aabb_overlap;
    zero_input.nearest_surface_point_inside_cell =
        item.nearest_surface_point_inside_cell;
    zero_input.block_lookup_suspicious = item.block_lookup_suspicious;
    zero_input.reference_sign_suspicious = item.reference_sign_suspicious;
    zero_input.sign_epsilon = options.sign_epsilon;
    zero_input.exact_center_phi = item.exact_center_phi;
    zero_input.exact_corner_phi.assign(
        std::begin(item.exact_corner_phi),
        std::end(item.exact_corner_phi));
    zero_input.exact_stencil_phi = item.exact_stencil_phi;
    const CellZeroCrossingResult zero =
        CellZeroCrossingDiagnostic::diagnose(zero_input);
    item.likely_zero_crossing_inside_cell =
        zero.likely_zero_crossing_inside_cell;
    item.zero_crossing_category = zero.category;
    if (zero.category == "surface_crossing_with_same_sign_corners") {
      ++result.surface_crossing_with_same_sign_corners_count;
    } else if (zero.category == "surface_crossing_with_mixed_corners") {
      ++result.surface_crossing_with_mixed_corners_count;
    } else if (zero.category == "no_surface_crossing_but_sign_mismatch") {
      ++result.no_surface_crossing_but_sign_mismatch_count;
    }

    const auto exact_phi_fn = [&](const Vector3& p) {
      return exactPhi(&sampler, p);
    };
    for (int subgrid : options.local_subgrids) {
      const LocalExactSubcellProbeResult probe =
          LocalExactSubcellProbe::probe(
              item.cell_aabb,
              source.point,
              item.sdf_sign,
              item.reference_sign,
              subgrid,
              exact_phi_fn,
              options.sign_epsilon);
      item.local_subgrid_results[subgrid] = probe;
      if (probe.fixed_mismatch) {
        ++subgrid_summaries[subgrid].fixed_count;
      }
      if (probe.remaining_mismatch) {
        ++subgrid_summaries[subgrid].remaining_mismatch_count;
      }
    }

    addStats(&pattern_bins, item.corner_pattern_category, item);
    addStats(&zero_bins, item.zero_crossing_category, item);
    ++result.sign_mismatch_count;
    if (item.false_inside) {
      ++result.false_inside_count;
    }
    if (item.false_outside) {
      ++result.false_outside_count;
    }
    result.samples.push_back(std::move(item));
  }

  result.sample_count = result.samples.size();
  result.p95_abs_error = percentile(errors, 0.95);
  result.corner_pattern_stats = finalizeStats(pattern_bins);
  result.zero_crossing_stats = finalizeStats(zero_bins);
  for (const auto& item : subgrid_summaries) {
    result.subgrid_summaries.push_back(item.second);
  }
  result.false_inside_major_corner_pattern =
      majorityPattern(false_inside_patterns);
  result.false_outside_major_corner_pattern =
      majorityPattern(false_outside_patterns);
  result.cell_resolution_insufficient_likely =
      result.sample_count > 0 &&
      static_cast<double>(
          result.surface_crossing_with_same_sign_corners_count +
          result.surface_crossing_with_mixed_corners_count) /
              static_cast<double>(result.sample_count) >=
          0.5;
  return result;
}

}  // namespace adasdf

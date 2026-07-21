#include "adasdf/narrowband/NarrowBandBrickQuery.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& start, const Clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

bool containsPoint(const AABB& box, const Vector3& p) {
  const double eps = 1.0e-12;
  return box.valid && p.x >= box.min.x - eps && p.x <= box.max.x + eps &&
         p.y >= box.min.y - eps && p.y <= box.max.y + eps &&
         p.z >= box.min.z - eps && p.z <= box.max.z + eps;
}

Vector3 clampToAABB(const AABB& box, const Vector3& p) {
  return {
      std::clamp(p.x, box.min.x, box.max.x),
      std::clamp(p.y, box.min.y, box.max.y),
      std::clamp(p.z, box.min.z, box.max.z)};
}

double distance(const Vector3& a, const Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double finiteOrZero(double value) {
  return std::isfinite(value) ? value : 0.0;
}

double axisCoord(double p, double origin, double spacing, int n) {
  if (!(spacing > 0.0) || n <= 1) {
    return 0.0;
  }
  return std::clamp((p - origin) / spacing, 0.0, static_cast<double>(n - 1));
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

void updatePhiStats(NarrowBandBrickQueryStats* stats, double phi) {
  if (stats == nullptr) {
    return;
  }
  if (stats->sample_count == 0) {
    stats->phi_min = phi;
    stats->phi_max = phi;
  } else {
    stats->phi_min = std::min(stats->phi_min, phi);
    stats->phi_max = std::max(stats->phi_max, phi);
  }
  ++stats->sample_count;
  stats->phi_sum += phi;
  stats->phi_mean = stats->phi_sum / static_cast<double>(stats->sample_count);
}

double sampleRecord(
    const NarrowBandBrickRecord& record,
    const Vector3& point,
    NarrowBandBrickQueryStats* stats) {
  const AdaptiveSDFBlock& block = *record.block;
  const Clock::time_point cell_start = Clock::now();
  const double x = axisCoord(point.x, block.origin.x, block.spacing.x, block.nx);
  const double y = axisCoord(point.y, block.origin.y, block.spacing.y, block.ny);
  const double z = axisCoord(point.z, block.origin.z, block.spacing.z, block.nz);
  const int i0 = static_cast<int>(std::floor(x));
  const int j0 = static_cast<int>(std::floor(y));
  const int k0 = static_cast<int>(std::floor(z));
  const int i1 = std::min(i0 + 1, block.nx - 1);
  const int j1 = std::min(j0 + 1, block.ny - 1);
  const int k1 = std::min(k0 + 1, block.nz - 1);
  const double tx = x - static_cast<double>(i0);
  const double ty = y - static_cast<double>(j0);
  const double tz = z - static_cast<double>(k0);
  if (stats != nullptr) {
    stats->timing.cell_index_time_ms += elapsedMs(cell_start, Clock::now());
  }

  const Clock::time_point data_start = Clock::now();
  const double c000 =
      finiteOrZero(block.phi[valueIndex(i0, j0, k0, block.nx, block.ny)]);
  const double c100 =
      finiteOrZero(block.phi[valueIndex(i1, j0, k0, block.nx, block.ny)]);
  const double c010 =
      finiteOrZero(block.phi[valueIndex(i0, j1, k0, block.nx, block.ny)]);
  const double c110 =
      finiteOrZero(block.phi[valueIndex(i1, j1, k0, block.nx, block.ny)]);
  const double c001 =
      finiteOrZero(block.phi[valueIndex(i0, j0, k1, block.nx, block.ny)]);
  const double c101 =
      finiteOrZero(block.phi[valueIndex(i1, j0, k1, block.nx, block.ny)]);
  const double c011 =
      finiteOrZero(block.phi[valueIndex(i0, j1, k1, block.nx, block.ny)]);
  const double c111 =
      finiteOrZero(block.phi[valueIndex(i1, j1, k1, block.nx, block.ny)]);
  if (stats != nullptr) {
    stats->timing.data_access_time_ms += elapsedMs(data_start, Clock::now());
    stats->tensor_nodes_touched += 8;
  }

  const Clock::time_point interp_start = Clock::now();
  const double c00 = lerp(c000, c100, tx);
  const double c10 = lerp(c010, c110, tx);
  const double c01 = lerp(c001, c101, tx);
  const double c11 = lerp(c011, c111, tx);
  const double c0 = lerp(c00, c10, ty);
  const double c1 = lerp(c01, c11, ty);
  const double phi = finiteOrZero(lerp(c0, c1, tz));
  if (stats != nullptr) {
    stats->timing.interpolation_time_ms +=
        elapsedMs(interp_start, Clock::now());
  }
  return phi;
}

}  // namespace

double NarrowBandBrickQueryStats::averageBlocksCheckedPerQuery() const {
  return block_lookup_count == 0
      ? 0.0
      : static_cast<double>(blocks_checked) /
            static_cast<double>(block_lookup_count);
}

double NarrowBandBrickQueryStats::averageTensorNodesTouchedPerQuery() const {
  return sample_count == 0
      ? 0.0
      : static_cast<double>(tensor_nodes_touched) /
            static_cast<double>(sample_count);
}

NarrowBandBrickQueryResult NarrowBandBrickQuery::samplePhi(
    const NarrowBandBrickIndex& index,
    const Vector3& point,
    NarrowBandBrickQueryStats* stats) {
  NarrowBandBrickQueryResult result;
  if (!index.valid()) {
    return result;
  }

  const bool outside = !containsPoint(index.bounds(), point);
  const Vector3 query_point = outside ? clampToAABB(index.bounds(), point) : point;

  NarrowBandBrickLookupStats lookup_stats;
  const Clock::time_point lookup_start = Clock::now();
  const NarrowBandBrickRecord* record = index.find(query_point, &lookup_stats);
  if (stats != nullptr) {
    stats->timing.block_lookup_time_ms +=
        elapsedMs(lookup_start, Clock::now());
    stats->block_lookup_count += lookup_stats.block_lookup_count;
    stats->block_lookup_miss_count += lookup_stats.block_lookup_miss_count;
    stats->blocks_checked += lookup_stats.blocks_checked;
    stats->out_of_domain_count += outside ? 1 : 0;
  }

  double phi = 0.0;
  if (record != nullptr && record->block != nullptr) {
    phi = sampleRecord(*record, query_point, stats);
    result.success = true;
    result.brick_id = record->brick_id;
  } else {
    result.success = true;
  }
  if (outside) {
    phi = std::abs(phi) + distance(point, query_point);
    result.out_of_domain = true;
  }
  result.phi = finiteOrZero(phi);
  updatePhiStats(stats, result.phi);
  return result;
}

std::vector<double> NarrowBandBrickQuery::queryPhi(
    const NarrowBandBrickIndex& index,
    const std::vector<Vector3>& points,
    NarrowBandBrickQueryStats* stats) {
  std::vector<double> out;
  out.reserve(points.size());
  for (const Vector3& point : points) {
    out.push_back(samplePhi(index, point, stats).phi);
  }
  return out;
}

}  // namespace adasdf


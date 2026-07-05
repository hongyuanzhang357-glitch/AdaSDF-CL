#include "adasdf/sampling/ContactBandMarker.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include "adasdf/acceleration/TriangleAABB.h"
#include "adasdf/geometry/BoxTriangleDistance.h"

namespace adasdf {
namespace {

double elapsedMs(
    const std::chrono::steady_clock::time_point& begin,
    const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

AABB cellAABB(
    const AABB& block_bounds,
    int i,
    int j,
    int k,
    int cell_count) {
  const double inv =
      cell_count <= 0 ? 0.0 : 1.0 / static_cast<double>(cell_count);
  const auto axis_min = [&](double lo, double hi, int index) {
    return lo + (hi - lo) * static_cast<double>(index) * inv;
  };
  const auto axis_max = [&](double lo, double hi, int index) {
    return lo + (hi - lo) * static_cast<double>(index + 1) * inv;
  };
  AABB box;
  box.valid = block_bounds.valid;
  box.min = {
      axis_min(block_bounds.min.x, block_bounds.max.x, i),
      axis_min(block_bounds.min.y, block_bounds.max.y, j),
      axis_min(block_bounds.min.z, block_bounds.max.z, k)};
  box.max = {
      axis_max(block_bounds.min.x, block_bounds.max.x, i),
      axis_max(block_bounds.min.y, block_bounds.max.y, j),
      axis_max(block_bounds.min.z, block_bounds.max.z, k)};
  return box;
}

TriangleAABB toTriangleAABB(const AABB& box) {
  return {box.min, box.max, box.valid};
}

TriangleAABB expandAABB(const AABB& box, double margin) {
  TriangleAABB out = toTriangleAABB(box);
  out.min.x -= margin;
  out.min.y -= margin;
  out.min.z -= margin;
  out.max.x += margin;
  out.max.y += margin;
  out.max.z += margin;
  return out;
}

bool overlaps(const TriangleAABB& a, const TriangleAABB& b) {
  if (!a.valid || !b.valid) {
    return false;
  }
  return a.min.x <= b.max.x && a.max.x >= b.min.x &&
         a.min.y <= b.max.y && a.max.y >= b.min.y &&
         a.min.z <= b.max.z && a.max.z >= b.min.z;
}

double cellDiagonal(const AABB& box) {
  if (!box.valid) {
    return 0.0;
  }
  const Vector3 d = box.max - box.min;
  return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

double effectiveBand(const AABB& cell, const ContactBandOptions& options) {
  const double base_band =
      std::max(std::max(0.0, options.contact_band_width),
               std::max(0.0, options.marker_min_band));
  double band =
      base_band * std::max(0.0, options.marker_safety_factor) +
      std::max(0.0, options.marker_cell_size_factor) * cellDiagonal(cell);
  if (options.marker_max_band > 0.0) {
    band = std::min(band, options.marker_max_band);
  }
  return std::max(0.0, band);
}

bool validTriangleIndex(const TriangleMesh& mesh, int triangle_index) {
  return triangle_index >= 0 &&
         static_cast<std::size_t>(triangle_index) < mesh.triangles.size();
}

struct CellMarkResult {
  bool mark = false;
  std::size_t candidates = 0;
  std::size_t refined = 0;
  std::size_t rejected = 0;
  double refinement_time_ms = 0.0;
  double box_triangle_distance_time_ms = 0.0;
  double triangle_bvh_query_time_ms = 0.0;
};

bool distanceAccepts(
    const BoxTriangleDistanceResult& distance,
    double band,
    double uncertainty_margin,
    ContactBandMarkerMode mode) {
  if (distance.lower_bound_distance > band + uncertainty_margin) {
    return false;
  }
  if (distance.intersects || distance.approximate_distance <= band) {
    return true;
  }
  if (mode == ContactBandMarkerMode::Hybrid &&
      distance.approximate_distance <= band + uncertainty_margin) {
    return true;
  }
  return false;
}

CellMarkResult markCellWithBVH(
    const TriangleBVH& bvh,
    const AABB& cell,
    const ContactBandOptions& options) {
  CellMarkResult result;
  const auto query0 = std::chrono::steady_clock::now();
  const auto finish = [&]() {
    const auto query1 = std::chrono::steady_clock::now();
    result.triangle_bvh_query_time_ms =
        std::max(0.0, elapsedMs(query0, query1) -
                          result.box_triangle_distance_time_ms);
    return result;
  };
  if (!bvh.isValid() || bvh.empty() || bvh.mesh() == nullptr) {
    return finish();
  }
  const TriangleMesh& mesh = *bvh.mesh();
  const double band =
      options.marker_mode == ContactBandMarkerMode::ConservativeAABB
          ? std::max(0.0, options.contact_band_width)
          : effectiveBand(cell, options);
  const TriangleAABB broadphase_cell =
      expandAABB(cell, std::max(0.0, options.contact_band_width));
  const std::vector<TriangleBVHNode>& nodes = bvh.nodes();
  const std::vector<int>& indices = bvh.triangleIndices();
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int node_index = stack.back();
    stack.pop_back();
    if (node_index < 0 ||
        node_index >= static_cast<int>(nodes.size())) {
      continue;
    }
    const TriangleBVHNode& node = nodes[static_cast<std::size_t>(node_index)];
    if (!overlaps(node.bounds, broadphase_cell)) {
      continue;
    }
    if (!node.isLeaf()) {
      stack.push_back(node.left);
      stack.push_back(node.right);
      continue;
    }
    for (std::size_t offset = 0; offset < node.count; ++offset) {
      const std::size_t ref_index = node.start + offset;
      if (ref_index >= indices.size()) {
        continue;
      }
      const int triangle_index = indices[ref_index];
      if (!validTriangleIndex(mesh, triangle_index)) {
        continue;
      }
      const MeshTriangle& triangle =
          mesh.triangles[static_cast<std::size_t>(triangle_index)];
      const TriangleAABB tri_box = makeMeshTriangleAABB(mesh, triangle);
      if (!overlaps(tri_box, broadphase_cell)) {
        continue;
      }
      ++result.candidates;
      if (options.marker_mode == ContactBandMarkerMode::ConservativeAABB) {
        result.mark = true;
        return finish();
      }
      ++result.refined;
      const auto refine0 = std::chrono::steady_clock::now();
      const BoxTriangleDistanceResult distance =
          BoxTriangleDistance::estimate(cell, mesh, triangle);
      const auto refine1 = std::chrono::steady_clock::now();
      const double distance_time = elapsedMs(refine0, refine1);
      result.refinement_time_ms += distance_time;
      result.box_triangle_distance_time_ms += distance_time;
      const double uncertainty =
          options.marker_mode == ContactBandMarkerMode::Hybrid
              ? 0.25 * cellDiagonal(cell)
              : 0.0;
      if (distanceAccepts(distance, band, uncertainty, options.marker_mode)) {
        result.mark = true;
        return finish();
      }
      ++result.rejected;
    }
  }
  return finish();
}

void markNode(std::vector<std::uint8_t>* values, std::size_t index) {
  if (values != nullptr && index < values->size()) {
    (*values)[index] = 1;
  }
}

void recount(ContactBandMask* mask) {
  mask->exact_required_count = 0;
  mask->contact_band_node_count = 0;
  mask->halo_node_count = 0;
  for (std::size_t i = 0; i < mask->exact_required.size(); ++i) {
    if (mask->exact_required[i] != 0) {
      ++mask->exact_required_count;
    }
    if (mask->contact_band_node[i] != 0) {
      ++mask->contact_band_node_count;
    }
    if (mask->halo_node[i] != 0) {
      ++mask->halo_node_count;
    }
  }
  mask->marked_node_count = mask->contact_band_node_count;
  mask->candidate_triangle_count =
      mask->candidate_triangle_aabb_overlap_count;
  mask->refined_candidate_count = mask->distance_refined_cell_count;
  mask->rejected_candidate_count = mask->distance_rejected_cell_count;
  mask->accepted_contact_cell_count = mask->marked_cell_count;
  mask->marker_refinement_time_ms = mask->distance_refinement_time_ms;
  if (mask->candidate_triangle_aabb_overlap_count > 0) {
    mask->overmark_ratio_estimate =
        static_cast<double>(mask->distance_rejected_cell_count) /
        static_cast<double>(mask->candidate_triangle_aabb_overlap_count);
    mask->marker_false_positive_proxy = mask->overmark_ratio_estimate;
  }
}

bool anyContactBandNode(const ContactBandMask& mask) {
  return std::any_of(
      mask.contact_band_node.begin(),
      mask.contact_band_node.end(),
      [](std::uint8_t value) { return value != 0; });
}

void expandContactNodesToExact(
    ContactBandMask* mask,
    int n,
    int layers,
    bool mark_halo) {
  if (mask == nullptr || layers <= 0) {
    return;
  }
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        const std::size_t index = gridIndex(i, j, k, n, n);
        if (mask->contact_band_node[index] == 0) {
          continue;
        }
        for (int dz = -layers; dz <= layers; ++dz) {
          for (int dy = -layers; dy <= layers; ++dy) {
            for (int dx = -layers; dx <= layers; ++dx) {
              const int x = i + dx;
              const int y = j + dy;
              const int z = k + dz;
              if (x < 0 || y < 0 || z < 0 || x >= n || y >= n || z >= n) {
                continue;
              }
              const std::size_t halo_index = gridIndex(x, y, z, n, n);
              if (mark_halo && mask->halo_node[halo_index] == 0) {
                ++mask->local_halo_node_count;
              }
              if (mark_halo) {
                markNode(&mask->halo_node, halo_index);
              }
              markNode(&mask->exact_required, halo_index);
            }
          }
        }
      }
    }
  }
}

void markGlobalHalo(ContactBandMask* mask, int n, int halo) {
  if (mask == nullptr || halo <= 0) {
    return;
  }
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        if (i < halo || j < halo || k < halo ||
            i >= n - halo || j >= n - halo || k >= n - halo) {
          const std::size_t index = gridIndex(i, j, k, n, n);
          if (mask->halo_node[index] == 0) {
            ++mask->global_halo_node_count;
          }
          markNode(&mask->halo_node, index);
          markNode(&mask->exact_required, index);
        }
      }
    }
  }
}

}  // namespace

const char* toString(ContactBandMarkerMode mode) {
  switch (mode) {
    case ContactBandMarkerMode::ConservativeAABB:
      return "conservative-aabb";
    case ContactBandMarkerMode::DistanceAware:
      return "distance-aware";
    case ContactBandMarkerMode::Hybrid:
      return "hybrid";
  }
  return "conservative-aabb";
}

bool parseContactBandMarkerMode(
    const std::string& value,
    ContactBandMarkerMode* mode) {
  if (mode == nullptr) {
    return false;
  }
  if (value == "conservative-aabb" || value == "conservative" ||
      value == "aabb") {
    *mode = ContactBandMarkerMode::ConservativeAABB;
    return true;
  }
  if (value == "distance-aware" || value == "distance") {
    *mode = ContactBandMarkerMode::DistanceAware;
    return true;
  }
  if (value == "hybrid") {
    *mode = ContactBandMarkerMode::Hybrid;
    return true;
  }
  return false;
}

ContactBandMask ContactBandMarker::markBlock(
    const AABB& block_bounds,
    int block_resolution,
    const TriangleBVH& triangle_bvh,
    const ContactBandOptions& options) {
  const auto marker0 = std::chrono::steady_clock::now();
  const int n = std::max(2, block_resolution);
  ContactBandMask mask;
  mask.marker_mode = toString(options.marker_mode);
  mask.nx = n;
  mask.ny = n;
  mask.nz = n;
  const std::size_t node_count =
      static_cast<std::size_t>(n) * static_cast<std::size_t>(n) *
      static_cast<std::size_t>(n);
  mask.exact_required.assign(node_count, 0);
  mask.contact_band_node.assign(node_count, 0);
  mask.halo_node.assign(node_count, 0);

  if (!block_bounds.valid || !triangle_bvh.isValid()) {
    return mask;
  }

  const int cell_count = std::max(1, n - 1);
  if (options.use_cell_aabb_marking && options.use_triangle_aabb_marking) {
    for (int k = 0; k < cell_count; ++k) {
      for (int j = 0; j < cell_count; ++j) {
        for (int i = 0; i < cell_count; ++i) {
          const AABB cell = cellAABB(block_bounds, i, j, k, cell_count);
          const CellMarkResult cell_result =
              markCellWithBVH(triangle_bvh, cell, options);
          mask.candidate_triangle_aabb_overlap_count +=
              cell_result.candidates;
          mask.candidate_triangle_count += cell_result.candidates;
          if (cell_result.candidates > 0) {
            ++mask.candidate_cell_count;
          }
          mask.distance_refined_cell_count += cell_result.refined;
          mask.refined_candidate_count += cell_result.refined;
          mask.distance_rejected_cell_count += cell_result.rejected;
          mask.rejected_candidate_count += cell_result.rejected;
          mask.distance_refinement_time_ms +=
              cell_result.refinement_time_ms;
          mask.marker_refinement_time_ms += cell_result.refinement_time_ms;
          mask.box_triangle_distance_time_ms +=
              cell_result.box_triangle_distance_time_ms;
          mask.triangle_bvh_query_time_ms +=
              cell_result.triangle_bvh_query_time_ms;
          if (!cell_result.mark) {
            continue;
          }
          ++mask.marked_cell_count;
          ++mask.accepted_contact_cell_count;
          for (int dz = 0; dz <= 1; ++dz) {
            for (int dy = 0; dy <= 1; ++dy) {
              for (int dx = 0; dx <= 1; ++dx) {
                markNode(
                    &mask.contact_band_node,
                    gridIndex(i + dx, j + dy, k + dz, n, n));
              }
            }
          }
        }
      }
    }
  }

  expandContactNodesToExact(
      &mask,
      n,
      std::max(0, options.contact_band_layers),
      false);

  const bool has_contact_band = anyContactBandNode(mask);
  if (has_contact_band && options.mark_halo_layers_exact) {
    const int halo = std::max(0, options.halo_exact_layers);
    if (options.local_halo_only || options.disable_global_halo) {
      expandContactNodesToExact(&mask, n, halo, true);
    } else {
      markGlobalHalo(&mask, n, halo);
    }
  }

  recount(&mask);
  const auto marker1 = std::chrono::steady_clock::now();
  mask.marker_time_ms = elapsedMs(marker0, marker1);
  return mask;
}

}  // namespace adasdf

#include "adasdf/sampling/ContactBandMarker.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "adasdf/acceleration/TriangleAABB.h"

namespace adasdf {
namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

TriangleAABB toTriangleAABB(const AABB& box) {
  return {box.min, box.max, box.valid};
}

TriangleAABB expandedCellAABB(
    const AABB& block_bounds,
    int i,
    int j,
    int k,
    int cell_count,
    double margin) {
  const double inv = cell_count <= 0 ? 0.0 : 1.0 / static_cast<double>(cell_count);
  const auto axis_min = [&](double lo, double hi, int index) {
    return lo + (hi - lo) * static_cast<double>(index) * inv - margin;
  };
  const auto axis_max = [&](double lo, double hi, int index) {
    return lo + (hi - lo) * static_cast<double>(index + 1) * inv + margin;
  };
  TriangleAABB box;
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

bool overlaps(const TriangleAABB& a, const TriangleAABB& b) {
  if (!a.valid || !b.valid) {
    return false;
  }
  return a.min.x <= b.max.x && a.max.x >= b.min.x &&
         a.min.y <= b.max.y && a.max.y >= b.min.y &&
         a.min.z <= b.max.z && a.max.z >= b.min.z;
}

bool bvhOverlapsCell(const TriangleBVH& bvh, const TriangleAABB& cell) {
  if (!bvh.isValid() || bvh.empty() || bvh.mesh() == nullptr) {
    return false;
  }
  const TriangleMesh& mesh = *bvh.mesh();
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
    if (!overlaps(node.bounds, cell)) {
      continue;
    }
    if (node.isLeaf()) {
      for (std::size_t offset = 0; offset < node.count; ++offset) {
        const std::size_t ref_index = node.start + offset;
        if (ref_index >= indices.size()) {
          continue;
        }
        const int triangle_index = indices[ref_index];
        if (triangle_index < 0 ||
            triangle_index >= static_cast<int>(mesh.triangles.size())) {
          continue;
        }
        const TriangleAABB tri_box =
            makeMeshTriangleAABB(
                mesh,
                mesh.triangles[static_cast<std::size_t>(triangle_index)]);
        if (overlaps(tri_box, cell)) {
          return true;
        }
      }
    } else {
      stack.push_back(node.left);
      stack.push_back(node.right);
    }
  }
  return false;
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
}

}  // namespace

ContactBandMask ContactBandMarker::markBlock(
    const AABB& block_bounds,
    int block_resolution,
    const TriangleBVH& triangle_bvh,
    const ContactBandOptions& options) {
  const int n = std::max(2, block_resolution);
  ContactBandMask mask;
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
          const TriangleAABB cell = expandedCellAABB(
              block_bounds,
              i,
              j,
              k,
              cell_count,
              std::max(0.0, options.contact_band_width));
          if (!bvhOverlapsCell(triangle_bvh, cell)) {
            continue;
          }
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

  const int layers = std::max(0, options.contact_band_layers);
  for (int k = 0; k < n; ++k) {
    for (int j = 0; j < n; ++j) {
      for (int i = 0; i < n; ++i) {
        const std::size_t index = gridIndex(i, j, k, n, n);
        if (mask.contact_band_node[index] == 0) {
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
              markNode(&mask.exact_required, gridIndex(x, y, z, n, n));
            }
          }
        }
      }
    }
  }

  const bool has_contact_band =
      std::any_of(
          mask.contact_band_node.begin(),
          mask.contact_band_node.end(),
          [](std::uint8_t value) { return value != 0; });
  if (has_contact_band && options.mark_halo_layers_exact) {
    const int halo = std::max(0, options.halo_exact_layers);
    for (int k = 0; k < n; ++k) {
      for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
          if (i < halo || j < halo || k < halo ||
              i >= n - halo || j >= n - halo || k >= n - halo) {
            const std::size_t index = gridIndex(i, j, k, n, n);
            markNode(&mask.halo_node, index);
            markNode(&mask.exact_required, index);
          }
        }
      }
    }
  }

  recount(&mask);
  return mask;
}

}  // namespace adasdf

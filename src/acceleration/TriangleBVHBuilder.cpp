#include "adasdf/acceleration/TriangleBVHBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

namespace adasdf {
namespace {

double dot(const Vector3& a, const Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

bool makeRef(
    const TriangleMesh& mesh,
    int triangle_index,
    const TriangleBVHBuildOptions& options,
    TriangleBVHRef* ref) {
  if (ref == nullptr || triangle_index < 0 ||
      static_cast<std::size_t>(triangle_index) >= mesh.triangles.size()) {
    return false;
  }
  const MeshTriangle& tri = mesh.triangles[static_cast<std::size_t>(triangle_index)];
  if (!validIndex(mesh, tri.v0) || !validIndex(mesh, tri.v1) ||
      !validIndex(mesh, tri.v2)) {
    return false;
  }
  const Vector3 a = toVector3(mesh.vertices[tri.v0]);
  const Vector3 b = toVector3(mesh.vertices[tri.v1]);
  const Vector3 c = toVector3(mesh.vertices[tri.v2]);
  if (!a.allFinite() || !b.allFinite() || !c.allFinite()) {
    return false;
  }
  const Vector3 ab = b - a;
  const Vector3 ac = c - a;
  const double area = 0.5 * std::sqrt(std::max(0.0, dot(cross(ab, ac), cross(ab, ac))));
  if (options.ignore_degenerate && area <= options.degenerate_area_epsilon) {
    return false;
  }
  ref->triangle_index = triangle_index;
  ref->bounds = makeTriangleAABB(a, b, c);
  ref->centroid = (a + b + c) / 3.0;
  return isValid(ref->bounds) && ref->centroid.allFinite();
}

int largestAxis(const TriangleAABB& bounds) {
  const Vector3 e = extent(bounds);
  if (e.x >= e.y && e.x >= e.z) {
    return 0;
  }
  if (e.y >= e.z) {
    return 1;
  }
  return 2;
}

}  // namespace

class TriangleBVHInternalBuilder {
 public:
  TriangleBVHInternalBuilder(
      TriangleBVH* bvh,
      std::vector<TriangleBVHRef>* refs,
      TriangleBVHBuildReport* report)
      : bvh_(bvh), refs_(refs), report_(report) {}

  int build(std::size_t begin, std::size_t end, int depth) {
    const int node_index = static_cast<int>(bvh_->nodes_.size());
    bvh_->nodes_.push_back(TriangleBVHNode());
    TriangleBVHNode& node = bvh_->nodes_.back();

    node.bounds = makeEmptyTriangleAABB();
    for (std::size_t i = begin; i < end; ++i) {
      expandToInclude(&node.bounds, (*refs_)[i].bounds);
    }

    const std::size_t count = end - begin;
    report_->max_depth_used = std::max(report_->max_depth_used, depth);
    if (count <= static_cast<std::size_t>(std::max(1, bvh_->options_.max_leaf_size)) ||
        depth >= bvh_->options_.max_depth) {
      makeLeaf(node, begin, end);
      return node_index;
    }

    TriangleAABB centroid_bounds = makeEmptyTriangleAABB();
    for (std::size_t i = begin; i < end; ++i) {
      expandToInclude(&centroid_bounds, (*refs_)[i].centroid);
    }
    const int axis = largestAxis(centroid_bounds);
    std::stable_sort(
        refs_->begin() + static_cast<std::ptrdiff_t>(begin),
        refs_->begin() + static_cast<std::ptrdiff_t>(end),
        [axis](const TriangleBVHRef& lhs, const TriangleBVHRef& rhs) {
          const double lv = axisValue(lhs.centroid, axis);
          const double rv = axisValue(rhs.centroid, axis);
          if (lv == rv) {
            return lhs.triangle_index < rhs.triangle_index;
          }
          return lv < rv;
        });

    const std::size_t mid = begin + count / 2;
    if (mid == begin || mid == end) {
      makeLeaf(node, begin, end);
      return node_index;
    }

    const int left = build(begin, mid, depth + 1);
    const int right = build(mid, end, depth + 1);
    TriangleBVHNode& final_node =
        bvh_->nodes_[static_cast<std::size_t>(node_index)];
    final_node.left = left;
    final_node.right = right;
    final_node.start = 0;
    final_node.count = 0;
    return node_index;
  }

 private:
  void makeLeaf(TriangleBVHNode& node, std::size_t begin, std::size_t end) {
    node.start = bvh_->triangle_indices_.size();
    node.count = end - begin;
    for (std::size_t i = begin; i < end; ++i) {
      bvh_->triangle_indices_.push_back((*refs_)[i].triangle_index);
    }
    ++report_->leaf_count;
  }

  TriangleBVH* bvh_;
  std::vector<TriangleBVHRef>* refs_;
  TriangleBVHBuildReport* report_;
};

TriangleBVH TriangleBVHBuilder::build(
    const TriangleMesh& mesh,
    const TriangleBVHBuildOptions& options,
    TriangleBVHBuildReport* report_out) {
  TriangleBVHBuildReport report;
  const auto t0 = std::chrono::steady_clock::now();
  TriangleBVH bvh;
  bvh.mesh_ = &mesh;
  bvh.options_ = options;
  if (bvh.options_.max_leaf_size < 1) {
    bvh.options_.max_leaf_size = 1;
  }
  if (bvh.options_.max_depth < 1) {
    bvh.options_.max_depth = 1;
  }
  if (bvh.options_.method == TriangleBVHBuildMethod::SAHPreview) {
    report.warnings.push_back(
        "SAH preview is not implemented; using deterministic median split.");
  }

  std::vector<TriangleBVHRef> refs;
  refs.reserve(mesh.triangles.size());
  for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
    TriangleBVHRef ref;
    if (makeRef(mesh, static_cast<int>(i), bvh.options_, &ref)) {
      refs.push_back(ref);
    } else {
      ++report.skipped_triangle_count;
    }
  }
  report.triangle_count = refs.size();
  if (refs.empty()) {
    report.error_message = "TriangleBVHBuilder found no valid triangles.";
    report.node_count = 0;
    report.leaf_count = 0;
    const auto t1 = std::chrono::steady_clock::now();
    report.build_time_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    if (report_out != nullptr) {
      *report_out = report;
    }
    return bvh;
  }

  TriangleBVHInternalBuilder builder(&bvh, &refs, &report);
  builder.build(0, refs.size(), 0);
  report.node_count = bvh.nodes_.size();
  report.success = bvh.isValid();
  if (!report.success && report.error_message.empty()) {
    report.error_message = "TriangleBVHBuilder produced an invalid BVH.";
  }
  const auto t1 = std::chrono::steady_clock::now();
  report.build_time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  if (report_out != nullptr) {
    *report_out = report;
  }
  return bvh;
}

}  // namespace adasdf

#include "adasdf/mesh/TriangleMesh.h"

#include <algorithm>
#include <cmath>

namespace adasdf {

namespace {

double distance(const MeshVertex& a, const MeshVertex& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

Vector3 toVector3(const MeshVertex& vertex) {
  return {vertex.x, vertex.y, vertex.z};
}

MeshVertex toMeshVertex(const Vector3& vertex) {
  return {vertex.x, vertex.y, vertex.z};
}

bool TriangleMesh::empty() const {
  return vertices.empty() || triangles.empty();
}

std::size_t TriangleMesh::vertexCount() const {
  return vertices.size();
}

std::size_t TriangleMesh::triangleCount() const {
  return triangles.size();
}

MeshAABB TriangleMesh::aabb() const {
  MeshAABB box;
  if (vertices.empty()) {
    return box;
  }

  box.min = vertices.front();
  box.max = vertices.front();
  for (const MeshVertex& vertex : vertices) {
    box.min.x = std::min(box.min.x, vertex.x);
    box.min.y = std::min(box.min.y, vertex.y);
    box.min.z = std::min(box.min.z, vertex.z);
    box.max.x = std::max(box.max.x, vertex.x);
    box.max.y = std::max(box.max.y, vertex.y);
    box.max.z = std::max(box.max.z, vertex.z);
  }
  return box;
}

double TriangleMesh::diagonalLength() const {
  const MeshAABB box = aabb();
  return distance(box.min, box.max);
}

}  // namespace adasdf

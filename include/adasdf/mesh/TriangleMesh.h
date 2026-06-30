#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/geometry/Transform.h"

namespace adasdf {

struct MeshVertex {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct MeshTriangle {
  int v0 = -1;
  int v1 = -1;
  int v2 = -1;
  int original_face_id = -1;
};

struct MeshAABB {
  MeshVertex min;
  MeshVertex max;
};

Vector3 toVector3(const MeshVertex& vertex);
MeshVertex toMeshVertex(const Vector3& vertex);

class TriangleMesh {
 public:
  std::vector<MeshVertex> vertices;
  std::vector<MeshTriangle> triangles;

  bool empty() const;
  std::size_t vertexCount() const;
  std::size_t triangleCount() const;
  MeshAABB aabb() const;
  double diagonalLength() const;
};

}  // namespace adasdf

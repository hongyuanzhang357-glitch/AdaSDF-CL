#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/geometry/CollisionGeometry.h"

namespace adasdf {

struct Triangle {
  int v0 = 0;
  int v1 = 0;
  int v2 = 0;
};

class MeshModel : public CollisionGeometry {
 public:
  MeshModel() = default;
  explicit MeshModel(std::filesystem::path source_path);

  GeometryKind kind() const override {
    return GeometryKind::Mesh;
  }

  AABB localAABB() const override;
  std::string debugName() const override;

  const std::filesystem::path& sourcePath() const {
    return source_path_;
  }

  const std::vector<Vector3>& vertices() const {
    return vertices_;
  }

  const std::vector<Triangle>& triangles() const {
    return triangles_;
  }

  // TODO: connect loaders to the existing sdf::StlReader and future OBJ reader.
  static MeshModel fromFile(const std::filesystem::path& path);

 private:
  std::filesystem::path source_path_;
  std::vector<Vector3> vertices_;
  std::vector<Triangle> triangles_;
  AABB bounds_;
};

}  // namespace adasdf

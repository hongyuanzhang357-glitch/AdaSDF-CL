#include "adasdf/mesh/STLWriter.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {

namespace {

MeshVertex sub(const MeshVertex& a, const MeshVertex& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

MeshVertex cross(const MeshVertex& a, const MeshVertex& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
}

double norm(const MeshVertex& vertex) {
  return std::sqrt(vertex.x * vertex.x + vertex.y * vertex.y +
                   vertex.z * vertex.z);
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

MeshVertex normalFor(
    const MeshVertex& a,
    const MeshVertex& b,
    const MeshVertex& c) {
  MeshVertex n = cross(sub(b, a), sub(c, a));
  const double length = norm(n);
  if (!(length > 0.0) || !std::isfinite(length)) {
    return {};
  }
  n.x /= length;
  n.y /= length;
  n.z /= length;
  return n;
}

void setError(std::string* error_message, const std::string& message) {
  if (error_message != nullptr) {
    *error_message = message;
  }
}

}  // namespace

bool STLWriter::write(
    const std::string& path_string,
    const TriangleMesh& mesh,
    const STLWriteOptions& options,
    std::string* error_message) {
  if (options.binary) {
    setError(error_message, "binary STL writing is not implemented");
    return false;
  }
  if (mesh.vertices.empty() || mesh.triangles.empty()) {
    setError(error_message, "cannot write an empty STL mesh");
    return false;
  }

  const std::filesystem::path path(path_string);
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    setError(error_message, "failed to open STL output path: " + path_string);
    return false;
  }

  file << "solid " << options.solid_name << "\n";
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      setError(error_message, "mesh contains invalid triangle indices");
      return false;
    }
    const MeshVertex& a = mesh.vertices[triangle.v0];
    const MeshVertex& b = mesh.vertices[triangle.v1];
    const MeshVertex& c = mesh.vertices[triangle.v2];
    const MeshVertex n = normalFor(a, b, c);
    file << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
    file << "    outer loop\n";
    file << "      vertex " << a.x << " " << a.y << " " << a.z << "\n";
    file << "      vertex " << b.x << " " << b.y << " " << b.z << "\n";
    file << "      vertex " << c.x << " " << c.y << " " << c.z << "\n";
    file << "    endloop\n";
    file << "  endfacet\n";
  }
  file << "endsolid " << options.solid_name << "\n";
  if (!file) {
    setError(error_message, "failed while writing STL output");
    return false;
  }
  return true;
}

}  // namespace adasdf

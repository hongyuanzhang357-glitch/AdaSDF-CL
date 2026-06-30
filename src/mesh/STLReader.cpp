#include "adasdf/mesh/STLReader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace adasdf {

namespace {

struct VertexKey {
  long long x = 0;
  long long y = 0;
  long long z = 0;

  bool operator==(const VertexKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

struct VertexKeyHash {
  std::size_t operator()(const VertexKey& key) const {
    std::size_t h = std::hash<long long>{}(key.x);
    h ^= std::hash<long long>{}(key.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<long long>{}(key.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
  }
};

class VertexMerger {
 public:
  explicit VertexMerger(const STLReadOptions& options) : options_(options) {}

  int add(TriangleMesh& mesh, const MeshVertex& vertex) {
    if (!options_.merge_duplicate_vertices) {
      mesh.vertices.push_back(vertex);
      return static_cast<int>(mesh.vertices.size() - 1);
    }

    const VertexKey key = makeKey(vertex, options_.vertex_merge_tolerance);
    const auto found = indices_.find(key);
    if (found != indices_.end()) {
      return found->second;
    }
    mesh.vertices.push_back(vertex);
    const int id = static_cast<int>(mesh.vertices.size() - 1);
    indices_[key] = id;
    return id;
  }

 private:
  static VertexKey makeKey(const MeshVertex& vertex, double tolerance) {
    if (!(tolerance > 0.0) || !std::isfinite(tolerance)) {
      tolerance = std::numeric_limits<double>::epsilon();
    }
    return {
        static_cast<long long>(std::llround(vertex.x / tolerance)),
        static_cast<long long>(std::llround(vertex.y / tolerance)),
        static_cast<long long>(std::llround(vertex.z / tolerance))};
  }

  STLReadOptions options_;
  std::unordered_map<VertexKey, int, VertexKeyHash> indices_;
};

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string trim(const std::string& value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::vector<unsigned char> readBytes(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::vector<unsigned char>(
      std::istreambuf_iterator<char>(file),
      std::istreambuf_iterator<char>());
}

std::uint32_t readU32LE(const unsigned char* bytes) {
  return (static_cast<std::uint32_t>(bytes[0])) |
         (static_cast<std::uint32_t>(bytes[1]) << 8) |
         (static_cast<std::uint32_t>(bytes[2]) << 16) |
         (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::uint16_t readU16LE(const unsigned char* bytes) {
  return static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(bytes[0]) |
      (static_cast<std::uint16_t>(bytes[1]) << 8));
}

float readF32LE(const unsigned char* bytes) {
  const std::uint32_t bits = readU32LE(bytes);
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

bool binarySizeMatches(const std::vector<unsigned char>& bytes) {
  if (bytes.size() < 84) {
    return false;
  }
  const std::uint32_t count = readU32LE(bytes.data() + 80);
  const std::uint64_t expected =
      84ull + static_cast<std::uint64_t>(count) * 50ull;
  return expected == bytes.size();
}

STLReadResult parseAscii(
    const std::filesystem::path& path,
    const STLReadOptions& options,
    bool allow_failure) {
  STLReadResult result;
  result.is_binary = false;

  std::ifstream file(path);
  if (!file) {
    result.error_message = "failed to open STL file";
    return result;
  }

  VertexMerger merger(options);
  std::vector<MeshVertex> pending;
  std::string line;
  int face_id = 0;
  std::size_t malformed_vertex_lines = 0;

  while (std::getline(file, line)) {
    const std::string stripped = trim(line);
    const std::string lowered = lowercase(stripped);
    if (lowered.rfind("vertex", 0) != 0) {
      continue;
    }

    std::istringstream in(stripped);
    std::string token;
    MeshVertex vertex;
    if (!(in >> token >> vertex.x >> vertex.y >> vertex.z)) {
      ++malformed_vertex_lines;
      continue;
    }
    pending.push_back(vertex);
    if (pending.size() == 3) {
      MeshTriangle triangle;
      triangle.v0 = merger.add(result.mesh, pending[0]);
      triangle.v1 = merger.add(result.mesh, pending[1]);
      triangle.v2 = merger.add(result.mesh, pending[2]);
      triangle.original_face_id = face_id++;
      result.mesh.triangles.push_back(triangle);
      pending.clear();
    }
  }

  result.raw_triangle_count = result.mesh.triangles.size();
  result.unique_vertex_count = result.mesh.vertices.size();
  result.success = result.raw_triangle_count > 0 && pending.empty();
  if (!result.success) {
    std::ostringstream msg;
    if (result.raw_triangle_count == 0) {
      msg << "ASCII STL contains no complete triangles";
    } else {
      msg << "ASCII STL ended with incomplete facet";
    }
    if (malformed_vertex_lines > 0) {
      msg << "; malformed vertex lines: " << malformed_vertex_lines;
    }
    result.error_message = msg.str();
    if (allow_failure) {
      result.mesh = {};
      result.raw_triangle_count = 0;
      result.unique_vertex_count = 0;
    }
  }
  return result;
}

STLReadResult parseBinary(
    const std::vector<unsigned char>& bytes,
    const STLReadOptions& options) {
  STLReadResult result;
  result.is_binary = true;
  if (bytes.size() < 84) {
    result.error_message = "binary STL is smaller than the 84-byte header";
    return result;
  }
  const std::uint32_t triangle_count = readU32LE(bytes.data() + 80);
  const std::uint64_t expected =
      84ull + static_cast<std::uint64_t>(triangle_count) * 50ull;
  if (expected != bytes.size()) {
    std::ostringstream msg;
    msg << "binary STL size mismatch: header declares " << triangle_count
        << " triangles, expected " << expected << " bytes, got "
        << bytes.size();
    result.error_message = msg.str();
    return result;
  }

  VertexMerger merger(options);
  std::size_t offset = 84;
  for (std::uint32_t i = 0; i < triangle_count; ++i) {
    offset += 12;  // normal
    std::array<MeshVertex, 3> vertices;
    for (MeshVertex& vertex : vertices) {
      vertex.x = readF32LE(bytes.data() + offset);
      vertex.y = readF32LE(bytes.data() + offset + 4);
      vertex.z = readF32LE(bytes.data() + offset + 8);
      offset += 12;
    }
    (void)readU16LE(bytes.data() + offset);
    offset += 2;

    MeshTriangle triangle;
    triangle.v0 = merger.add(result.mesh, vertices[0]);
    triangle.v1 = merger.add(result.mesh, vertices[1]);
    triangle.v2 = merger.add(result.mesh, vertices[2]);
    triangle.original_face_id = static_cast<int>(i);
    result.mesh.triangles.push_back(triangle);
  }

  result.raw_triangle_count = triangle_count;
  result.unique_vertex_count = result.mesh.vertices.size();
  result.success = true;
  return result;
}

bool startsWithSolid(const std::vector<unsigned char>& bytes) {
  if (bytes.size() < 5) {
    return false;
  }
  const std::string prefix(
      reinterpret_cast<const char*>(bytes.data()),
      reinterpret_cast<const char*>(bytes.data()) + 5);
  return lowercase(prefix) == "solid";
}

}  // namespace

STLReadResult STLReader::read(
    const std::string& path_string,
    const STLReadOptions& options) {
  const std::filesystem::path path(path_string);
  STLReadResult result;
  if (!std::filesystem::exists(path)) {
    result.error_message = "STL file does not exist: " + path_string;
    return result;
  }
  if (!std::filesystem::is_regular_file(path)) {
    result.error_message = "STL path is not a regular file: " + path_string;
    return result;
  }

  const std::vector<unsigned char> bytes = readBytes(path);
  if (bytes.empty()) {
    result.error_message = "STL file is empty: " + path_string;
    return result;
  }

  const bool size_matches_binary = binarySizeMatches(bytes);
  if (startsWithSolid(bytes)) {
    STLReadResult ascii = parseAscii(path, options, true);
    if (ascii.success || !size_matches_binary) {
      return ascii;
    }
  }

  if (size_matches_binary) {
    return parseBinary(bytes, options);
  }
  return parseAscii(path, options, false);
}

}  // namespace adasdf

#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
  std::filesystem::path stl;
  std::size_t samples = 20000;
  std::vector<double> offsets = {-0.004, -0.002, -0.001,
                                 0.001, 0.002, 0.004};
  std::string reference_sign_mode = "ray-majority";
  std::filesystem::path json;
  std::filesystem::path report;
  std::string case_id = "mesh_sign_diagnostic";
};

struct Result {
  std::string case_id;
  std::filesystem::path input_stl;
  std::size_t triangle_count = 0;
  std::size_t vertex_count = 0;
  double signed_volume = 0.0;
  std::size_t sample_count = 0;
  std::size_t ray_x_y_z_disagreement_count = 0;
  std::size_t normal_offset_disagreement_count = 0;
  std::size_t suspected_inverted_triangle_count = 0;
  std::size_t suspected_nonorientable_region_count = 0;
  bool sign_reference_reliable = false;
};

void usage() {
  std::cerr
      << "Usage: adasdf_diagnose_mesh_sign model.stl [--samples N] "
         "[--offsets a,b,c] [--reference-sign-mode ray-majority] "
         "[--json out.json] [--report out.md] [--case-id id]\n";
}

std::vector<double> parseList(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stod(item));
    }
  }
  return values;
}

bool hasValue(int i, int argc) {
  return i + 1 < argc;
}

bool parseArgs(int argc, char** argv, Options* options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else if (arg == "--samples" && hasValue(i, argc)) {
      options->samples = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--offsets" && hasValue(i, argc)) {
      options->offsets = parseList(argv[++i]);
    } else if (arg == "--reference-sign-mode" && hasValue(i, argc)) {
      options->reference_sign_mode = argv[++i];
    } else if (arg == "--json" && hasValue(i, argc)) {
      options->json = argv[++i];
    } else if (arg == "--report" && hasValue(i, argc)) {
      options->report = argv[++i];
    } else if (arg == "--case-id" && hasValue(i, argc)) {
      options->case_id = argv[++i];
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    } else if (options->stl.empty()) {
      options->stl = arg;
    } else {
      std::cerr << "Unexpected positional argument: " << arg << "\n";
      return false;
    }
  }
  return !options->stl.empty() && options->samples > 0 &&
         !options->offsets.empty();
}

double dot(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

adasdf::Vector3 cross(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  return {a.y * b.z - a.z * b.y,
          a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

double norm(const adasdf::Vector3& v) {
  return std::sqrt(std::max(0.0, dot(v, v)));
}

bool validIndex(const adasdf::TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

int raySign(
    const adasdf::TriangleBVH& bvh,
    const adasdf::Vector3& point,
    const adasdf::Vector3& direction) {
  const adasdf::BVHRay ray{point, direction};
  const adasdf::BVHRayIntersectionResult result =
      adasdf::BVHRayIntersectionQuery::countIntersections(bvh, ray);
  if (!result.success || result.ambiguous) {
    return 0;
  }
  return result.hit_count % 2 == 1 ? -1 : 1;
}

int majoritySign(int sx, int sy, int sz) {
  const int sum = sx + sy + sz;
  if (sum > 0) {
    return 1;
  }
  if (sum < 0) {
    return -1;
  }
  return 0;
}

double signedVolume(const adasdf::TriangleMesh& mesh) {
  double volume = 0.0;
  for (const adasdf::MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      continue;
    }
    const adasdf::Vector3 a = adasdf::toVector3(mesh.vertices[triangle.v0]);
    const adasdf::Vector3 b = adasdf::toVector3(mesh.vertices[triangle.v1]);
    const adasdf::Vector3 c = adasdf::toVector3(mesh.vertices[triangle.v2]);
    volume += dot(a, cross(b, c)) / 6.0;
  }
  return volume;
}

adasdf::Vector3 meshCenter(const adasdf::TriangleMesh& mesh) {
  const adasdf::MeshAABB box = mesh.aabb();
  return 0.5 * (adasdf::toVector3(box.min) + adasdf::toVector3(box.max));
}

std::size_t suspectedInvertedTriangles(const adasdf::TriangleMesh& mesh) {
  const adasdf::Vector3 center = meshCenter(mesh);
  std::size_t count = 0;
  for (const adasdf::MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      continue;
    }
    const adasdf::Vector3 a = adasdf::toVector3(mesh.vertices[triangle.v0]);
    const adasdf::Vector3 b = adasdf::toVector3(mesh.vertices[triangle.v1]);
    const adasdf::Vector3 c = adasdf::toVector3(mesh.vertices[triangle.v2]);
    const adasdf::Vector3 normal = cross(b - a, c - a);
    if (norm(normal) <= 0.0) {
      continue;
    }
    const adasdf::Vector3 centroid = (a + b + c) / 3.0;
    if (dot(normal, centroid - center) < 0.0) {
      ++count;
    }
  }
  return count;
}

Result diagnose(const adasdf::TriangleMesh& mesh, const Options& options) {
  Result result;
  result.case_id = options.case_id;
  result.input_stl = options.stl;
  result.triangle_count = mesh.triangles.size();
  result.vertex_count = mesh.vertices.size();
  result.signed_volume = signedVolume(mesh);
  result.suspected_inverted_triangle_count = suspectedInvertedTriangles(mesh);
  const adasdf::TriangleBVH bvh = adasdf::TriangleBVHBuilder::build(mesh);
  if (!bvh.isValid()) {
    throw std::runtime_error("failed to build TriangleBVH for sign diagnostic");
  }

  adasdf::NearSurfaceSampleOptions sample_options;
  sample_options.surface_sample_count = options.samples;
  sample_options.offsets = options.offsets;
  const adasdf::NearSurfaceSampleSet samples =
      adasdf::NearSurfaceSampleGenerator::generate(mesh, sample_options);
  result.sample_count = samples.samples.size();
  for (const adasdf::NearSurfaceSample& sample : samples.samples) {
    const int sx = raySign(bvh, sample.point, {1.0, 0.0, 0.0});
    const int sy = raySign(bvh, sample.point, {0.0, 1.0, 0.0});
    const int sz = raySign(bvh, sample.point, {0.0, 0.0, 1.0});
    if (!(sx == sy && sy == sz)) {
      ++result.ray_x_y_z_disagreement_count;
    }
    const int majority = majoritySign(sx, sy, sz);
    const int expected =
        sample.offset < 0.0 ? -1 : (sample.offset > 0.0 ? 1 : 0);
    if (expected != 0 && majority != 0 && expected != majority) {
      ++result.normal_offset_disagreement_count;
    }
  }
  result.suspected_nonorientable_region_count =
      result.ray_x_y_z_disagreement_count;
  const double ray_rate =
      result.sample_count == 0
          ? 1.0
          : static_cast<double>(result.ray_x_y_z_disagreement_count) /
                static_cast<double>(result.sample_count);
  const double normal_rate =
      result.sample_count == 0
          ? 1.0
          : static_cast<double>(result.normal_offset_disagreement_count) /
                static_cast<double>(result.sample_count);
  result.sign_reference_reliable = result.sample_count > 0 &&
                                   ray_rate < 0.05 && normal_rate < 0.50;
  return result;
}

void createParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

bool writeJson(const std::filesystem::path& path, const Result& result) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  const auto escapeJson = [](const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char ch : text) {
      switch (ch) {
        case '\\':
          escaped += "\\\\";
          break;
        case '"':
          escaped += "\\\"";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\t':
          escaped += "\\t";
          break;
        default:
          escaped += ch;
          break;
      }
    }
    return escaped;
  };
  out << std::setprecision(17);
  out << "{\n";
  out << "  \"schema_id\": \"adasdf.mesh_sign_diagnostic.v1\",\n";
  out << "  \"case_id\": \"" << escapeJson(result.case_id) << "\",\n";
  out << "  \"input_stl\": \"" << escapeJson(result.input_stl.string()) << "\",\n";
  out << "  \"triangle_count\": " << result.triangle_count << ",\n";
  out << "  \"vertex_count\": " << result.vertex_count << ",\n";
  out << "  \"signed_volume\": " << result.signed_volume << ",\n";
  out << "  \"sample_count\": " << result.sample_count << ",\n";
  out << "  \"ray_x_y_z_disagreement_count\": "
      << result.ray_x_y_z_disagreement_count << ",\n";
  out << "  \"normal_offset_disagreement_count\": "
      << result.normal_offset_disagreement_count << ",\n";
  out << "  \"suspected_inverted_triangle_count\": "
      << result.suspected_inverted_triangle_count << ",\n";
  out << "  \"suspected_nonorientable_region_count\": "
      << result.suspected_nonorientable_region_count << ",\n";
  out << "  \"sign_reference_reliable\": "
      << (result.sign_reference_reliable ? "true" : "false") << "\n";
  out << "}\n";
  return true;
}

bool writeMarkdown(const std::filesystem::path& path, const Result& result) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(10);
  out << "# Mesh Sign Diagnostic\n\n";
  out << "- Case id: " << result.case_id << "\n";
  out << "- Input STL: " << result.input_stl.string() << "\n\n";
  out << "| metric | value |\n";
  out << "| --- | ---: |\n";
  out << "| triangle_count | " << result.triangle_count << " |\n";
  out << "| vertex_count | " << result.vertex_count << " |\n";
  out << "| signed_volume | " << result.signed_volume << " |\n";
  out << "| sample_count | " << result.sample_count << " |\n";
  out << "| ray_x_y_z_disagreement_count | "
      << result.ray_x_y_z_disagreement_count << " |\n";
  out << "| normal_offset_disagreement_count | "
      << result.normal_offset_disagreement_count << " |\n";
  out << "| suspected_inverted_triangle_count | "
      << result.suspected_inverted_triangle_count << " |\n";
  out << "| suspected_nonorientable_region_count | "
      << result.suspected_nonorientable_region_count << " |\n";
  out << "| sign_reference_reliable | "
      << (result.sign_reference_reliable ? "true" : "false") << " |\n";
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options;
    if (!parseArgs(argc, argv, &options)) {
      usage();
      return 2;
    }
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(options.stl.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }
    const Result result = diagnose(read.mesh, options);
    if (!options.json.empty() && !writeJson(options.json, result)) {
      std::cerr << "failed to write JSON\n";
      return 1;
    }
    if (!options.report.empty() && !writeMarkdown(options.report, result)) {
      std::cerr << "failed to write report\n";
      return 1;
    }
    std::cout << "mesh sign diagnostic completed\n";
    std::cout << "sample_count=" << result.sample_count << "\n";
    std::cout << "ray_x_y_z_disagreement_count="
              << result.ray_x_y_z_disagreement_count << "\n";
    std::cout << "normal_offset_disagreement_count="
              << result.normal_offset_disagreement_count << "\n";
    std::cout << "sign_reference_reliable="
              << (result.sign_reference_reliable ? "true" : "false")
              << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_diagnose_mesh_sign failed: " << exc.what() << "\n";
    return 1;
  }
}

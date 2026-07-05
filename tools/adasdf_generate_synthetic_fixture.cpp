#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Tri {
  Vec3 a;
  Vec3 b;
  Vec3 c;
};

Vec3 operator-(const Vec3& a, const Vec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(const Vec3& a, const Vec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x};
}

Vec3 normalOf(const Tri& tri) {
  Vec3 n = cross(tri.b - tri.a, tri.c - tri.a);
  const double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  if (len <= 1e-12) {
    return {0.0, 0.0, 1.0};
  }
  return {n.x / len, n.y / len, n.z / len};
}

void addQuad(
    std::vector<Tri>* tris,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d) {
  tris->push_back({a, b, c});
  tris->push_back({a, c, d});
}

void addBox(
    std::vector<Tri>* tris,
    double x0,
    double y0,
    double z0,
    double x1,
    double y1,
    double z1) {
  const Vec3 p000{x0, y0, z0};
  const Vec3 p100{x1, y0, z0};
  const Vec3 p010{x0, y1, z0};
  const Vec3 p110{x1, y1, z0};
  const Vec3 p001{x0, y0, z1};
  const Vec3 p101{x1, y0, z1};
  const Vec3 p011{x0, y1, z1};
  const Vec3 p111{x1, y1, z1};
  addQuad(tris, p000, p010, p110, p100);
  addQuad(tris, p001, p101, p111, p011);
  addQuad(tris, p000, p100, p101, p001);
  addQuad(tris, p010, p011, p111, p110);
  addQuad(tris, p000, p001, p011, p010);
  addQuad(tris, p100, p110, p111, p101);
}

Vec3 spherePoint(int lat, int lon, int lat_count, int lon_count, bool dense) {
  const double pi = 3.14159265358979323846;
  const double theta = pi * static_cast<double>(lat) /
                       static_cast<double>(lat_count);
  const double phi = 2.0 * pi * static_cast<double>(lon) /
                     static_cast<double>(lon_count);
  const double wave =
      dense ? 0.08 * std::sin(7.0 * theta) * std::cos(11.0 * phi)
            : 0.06 * std::sin(5.0 * theta) * std::cos(7.0 * phi);
  const double r = 0.55 + wave;
  return {
      r * std::sin(theta) * std::cos(phi),
      r * std::sin(theta) * std::sin(phi),
      r * std::cos(theta)};
}

std::vector<Tri> makeWavySphere(bool dense) {
  const int lat_count = dense ? 24 : 12;
  const int lon_count = dense ? 48 : 24;
  std::vector<Tri> tris;
  for (int lat = 0; lat < lat_count; ++lat) {
    for (int lon = 0; lon < lon_count; ++lon) {
      const int lon_next = (lon + 1) % lon_count;
      const Vec3 a = spherePoint(lat, lon, lat_count, lon_count, dense);
      const Vec3 b = spherePoint(lat + 1, lon, lat_count, lon_count, dense);
      const Vec3 c = spherePoint(lat + 1, lon_next, lat_count, lon_count, dense);
      const Vec3 d = spherePoint(lat, lon_next, lat_count, lon_count, dense);
      if (lat == 0) {
        tris.push_back({a, b, c});
      } else if (lat == lat_count - 1) {
        tris.push_back({a, b, d});
      } else {
        addQuad(&tris, a, b, c, d);
      }
    }
  }
  return tris;
}

std::vector<Tri> makeThinGapBox() {
  std::vector<Tri> tris;
  addBox(&tris, -0.55, -0.35, -0.2, -0.035, 0.35, 0.2);
  addBox(&tris, 0.035, -0.35, -0.2, 0.55, 0.35, 0.2);
  addBox(&tris, -0.55, -0.55, -0.08, 0.55, -0.42, 0.08);
  addBox(&tris, -0.55, 0.42, -0.08, 0.55, 0.55, 0.08);
  return tris;
}

std::vector<Tri> makeRobotLinkLike() {
  std::vector<Tri> tris;
  addBox(&tris, -0.75, -0.12, -0.08, 0.75, 0.12, 0.08);
  addBox(&tris, -0.95, -0.28, -0.07, -0.62, 0.28, 0.07);
  addBox(&tris, 0.62, -0.28, -0.07, 0.95, 0.28, 0.07);
  addBox(&tris, -0.12, -0.26, -0.11, 0.12, 0.26, 0.11);
  addBox(&tris, -0.38, -0.045, -0.12, 0.38, 0.045, 0.12);
  return tris;
}

std::vector<Tri> makeGearLikeTeeth() {
  const int teeth = 18;
  const double pi = 3.14159265358979323846;
  const double z0 = -0.06;
  const double z1 = 0.06;
  std::vector<Vec3> bottom;
  std::vector<Vec3> top;
  for (int i = 0; i < teeth * 2; ++i) {
    const double r = (i % 2 == 0) ? 0.55 : 0.68;
    const double a = 2.0 * pi * static_cast<double>(i) /
                     static_cast<double>(teeth * 2);
    bottom.push_back({r * std::cos(a), r * std::sin(a), z0});
    top.push_back({r * std::cos(a), r * std::sin(a), z1});
  }
  std::vector<Tri> tris;
  const Vec3 cb{0.0, 0.0, z0};
  const Vec3 ct{0.0, 0.0, z1};
  for (int i = 0; i < teeth * 2; ++i) {
    const int j = (i + 1) % (teeth * 2);
    tris.push_back({cb, bottom[j], bottom[i]});
    tris.push_back({ct, top[i], top[j]});
    addQuad(&tris, bottom[i], bottom[j], top[j], top[i]);
  }
  return tris;
}

bool writeStl(
    const std::string& path,
    const std::string& name,
    const std::vector<Tri>& tris) {
  std::ofstream out(path.c_str());
  if (!out) {
    return false;
  }
  out << "solid project_generated_" << name << "\n";
  for (const Tri& tri : tris) {
    const Vec3 n = normalOf(tri);
    out << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
    out << "    outer loop\n";
    out << "      vertex " << tri.a.x << " " << tri.a.y << " " << tri.a.z
        << "\n";
    out << "      vertex " << tri.b.x << " " << tri.b.y << " " << tri.b.z
        << "\n";
    out << "      vertex " << tri.c.x << " " << tri.c.y << " " << tri.c.z
        << "\n";
    out << "    endloop\n";
    out << "  endfacet\n";
  }
  out << "endsolid project_generated_" << name << "\n";
  return true;
}

bool writeKind(const std::string& kind, const std::string& path) {
  if (kind == "thin-gap-box") {
    return writeStl(path, "thin_gap_box", makeThinGapBox());
  }
  if (kind == "robot-link-like") {
    return writeStl(path, "robot_link_like", makeRobotLinkLike());
  }
  if (kind == "dense-wavy-sphere") {
    return writeStl(path, "dense_wavy_sphere", makeWavySphere(true));
  }
  if (kind == "gear-like-teeth") {
    return writeStl(path, "gear_like_teeth", makeGearLikeTeeth());
  }
  return false;
}

std::string joinPath(const std::string& dir, const std::string& name) {
  if (dir.empty()) {
    return name;
  }
  const char last = dir[dir.size() - 1];
  if (last == '/' || last == '\\') {
    return dir + name;
  }
  return dir + "/" + name;
}

void usage() {
  std::cout
      << "Usage: adasdf_generate_synthetic_fixture --kind KIND --out file.stl\n"
      << "       adasdf_generate_synthetic_fixture --all --out-dir DIR\n"
      << "Kinds: thin-gap-box, robot-link-like, dense-wavy-sphere, "
         "gear-like-teeth\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string kind;
  bool all = false;
  std::string out;
  std::string out_dir;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    }
    if (arg == "--all") {
      all = true;
    } else if (arg == "--kind" && i + 1 < argc) {
      kind = argv[++i];
    } else if (arg == "--out" && i + 1 < argc) {
      out = argv[++i];
    } else if (arg == "--out-dir" && i + 1 < argc) {
      out_dir = argv[++i];
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 1;
    }
  }

  if (all) {
    if (out_dir.empty()) {
      std::cerr << "--all requires --out-dir\n";
      return 1;
    }
    const std::pair<std::string, std::string> cases[] = {
        {"thin-gap-box", "thin_gap_box_ascii.stl"},
        {"robot-link-like", "robot_link_like_ascii.stl"},
        {"dense-wavy-sphere", "dense_wavy_sphere_ascii.stl"},
        {"gear-like-teeth", "gear_like_teeth_ascii.stl"},
    };
    for (const auto& item : cases) {
      if (!writeKind(item.first, joinPath(out_dir, item.second))) {
        std::cerr << "Failed to write fixture: " << item.first << "\n";
        return 2;
      }
    }
    return 0;
  }

  if (kind.empty() || out.empty()) {
    usage();
    return 1;
  }
  if (!writeKind(kind, out)) {
    std::cerr << "Unknown fixture kind or write failure: " << kind << "\n";
    return 2;
  }
  return 0;
}

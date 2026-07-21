#include <adasdf/adasdf.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_generate_aabb_samples input.stl --count N "
         "--seed N --domain mesh-aabb --csv samples.csv\n";
}

bool hasValue(int i, int argc) {
  return i + 1 < argc;
}

double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2 || std::string(argv[1]) == "--help") {
      usage();
      return argc < 2 ? 1 : 0;
    }
    const std::filesystem::path stl_path = argv[1];
    std::size_t count = 10000;
    std::uint32_t seed = 123;
    std::string domain = "mesh-aabb";
    std::filesystem::path csv_path;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--count" && hasValue(i, argc)) {
        count = static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if (arg == "--seed" && hasValue(i, argc)) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
      } else if (arg == "--domain" && hasValue(i, argc)) {
        domain = argv[++i];
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }
    if (domain != "mesh-aabb") {
      std::cerr << "Only --domain mesh-aabb is supported in this prototype\n";
      return 1;
    }
    if (csv_path.empty()) {
      std::cerr << "--csv is required\n";
      return 1;
    }

    const adasdf::STLReadResult read = adasdf::STLReader::read(stl_path.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }
    const adasdf::MeshAABB aabb = read.mesh.aabb();
    if (!csv_path.parent_path().empty()) {
      std::filesystem::create_directories(csv_path.parent_path());
    }
    std::ofstream out(csv_path);
    if (!out) {
      std::cerr << "failed to open output CSV\n";
      return 1;
    }
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    out << "id,x,y,z,radius,object_id,link_id,group_id,weight,label\n";
    for (std::size_t i = 0; i < count; ++i) {
      const double x = lerp(aabb.min.x, aabb.max.x, unit(rng));
      const double y = lerp(aabb.min.y, aabb.max.y, unit(rng));
      const double z = lerp(aabb.min.z, aabb.max.z, unit(rng));
      out << i << "," << x << "," << y << "," << z
          << ",0,0,0,0,1,aabb\n";
    }
    std::cout << "sample_count=" << count << "\n";
    std::cout << "domain=mesh-aabb\n";
    std::cout << "csv=" << csv_path.string() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_generate_aabb_samples failed: " << exc.what() << "\n";
    return 1;
  }
}


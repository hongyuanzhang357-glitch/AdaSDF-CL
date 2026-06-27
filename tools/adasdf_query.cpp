#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_query model.sdfbin --point x y z\n"
      << "Queries one world-space point with the installed CPU SDF backend.\n";
}

bool hasValue(int index, int argc, int count) {
  return index + count < argc;
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc < 6) {
    usage();
    return 2;
  }

  const std::filesystem::path path = argv[1];
  Vector3 point{};
  bool point_set = false;

  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--point" && hasValue(i, argc, 3)) {
      point = {std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
      point_set = true;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  if (!point_set) {
    std::cerr << "adasdf_query: --point x y z is required.\n";
    usage();
    return 2;
  }
  if (!std::filesystem::exists(path)) {
    std::cerr << "adasdf_query: file does not exist: " << path.string() << "\n";
    return 2;
  }

  try {
    const auto model = SDFBinReader::read(path);
    if (!model->queryBackendAvailable()) {
      std::cerr
          << "adasdf_query: this build loaded metadata but has no query backend. "
          << "Rebuild with ADASDF_CL_USE_EXISTING_CORE=ON to sample existing .sdfbin files.\n";
      return 1;
    }

    const double distance = model->sampleDistance(point);
    const Vector3 gradient = model->sampleGradient(point);
    const Vector3 normal = model->sampleNormal(point);

    std::cout << "AdaSDF-CL point query\n";
    std::cout << "Path: " << path.string() << "\n";
    printVec("Point: ", point);
    std::cout << "Signed distance: " << distance << "\n";
    printVec("Gradient: ", gradient);
    printVec("Normal: ", normal);
    std::cout << "Query backend: " << model->metadata().query_backend << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_query failed: " << exc.what() << "\n";
    return 1;
  }
}

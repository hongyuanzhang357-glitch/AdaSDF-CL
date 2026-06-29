#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_make_demo_box output.sdfbin [options]\n"
      << "Options:\n"
      << "  --center x y z          Box center, default 0 0 0\n"
      << "  --half-extent hx hy hz  Box half extent, default 0.5 0.5 0.5\n"
      << "  --unit name             Unit label, default m\n";
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
  if (argc < 2) {
    usage();
    return 2;
  }

  const std::filesystem::path output = argv[1];
  Vector3 center{0.0, 0.0, 0.0};
  Vector3 half_extent{0.5, 0.5, 0.5};
  std::string unit = "m";

  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--center" && hasValue(i, argc, 3)) {
      center = {std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--half-extent" && hasValue(i, argc, 3)) {
      half_extent = {
          std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--unit" && hasValue(i, argc, 1)) {
      unit = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  try {
    auto model = AnalyticSDFModel::createBox(center, half_extent, unit);
    SDFBinWriter::write(output.string(), *model);
    auto reloaded = SDFBinReader::read(output);

    std::cout << "AdaSDF-CL demo box generator\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Shape: box\n";
    printVec("Center: ", center);
    printVec("Half extent: ", half_extent);
    std::cout << "Backend: " << AnalyticSDFModel::backendName() << "\n";
    std::cout << "Write status: success\n";
    std::cout << "Reload validation: "
              << (reloaded && reloaded->isValid() ? "success" : "failed") << "\n";
    return reloaded && reloaded->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_make_demo_box failed: " << exc.what() << "\n";
    return 1;
  }
}

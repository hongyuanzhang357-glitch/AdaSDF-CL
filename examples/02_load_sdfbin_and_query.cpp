#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

double length(const adasdf::Vector3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 2) {
    std::cout << "Usage: adasdf_load_sdfbin_and_query path/to/model.sdfbin\n";
    return 1;
  }

  try {
    const std::string path = argv[1];
    auto model = SDFBinReader::read(path);
    const AABB bounds = model->boundingBox();
    const Vector3 center = 0.5 * (bounds.min + bounds.max);

    std::cout << "AdaSDF-CL sdfbin query example\n";
    std::cout << "File: " << path << "\n";
    std::cout << "Valid: " << (model->isValid() ? "true" : "false") << "\n";
    printVec("AABB min: ", bounds.min);
    printVec("AABB max: ", bounds.max);
    std::cout << "Memory footprint: " << model->memoryFootprintBytes() << " bytes\n";
    std::cout << "Near-surface error: " << model->nearSurfaceError() << "\n";
    std::cout << "Backend: " << model->metadata().query_backend << "\n";

    const Vector3 query_points[] = {
        center,
        bounds.min,
        bounds.max,
        {center.x + 0.25 * (bounds.max.x - bounds.min.x), center.y, center.z}};

    for (const Vector3& p : query_points) {
      const double phi = model->sampleDistance(p);
      const Vector3 gradient = model->sampleGradient(p);
      const Vector3 normal = model->sampleNormal(p);
      std::cout << "\n";
      printVec("Query point: ", p);
      std::cout << "Signed distance: " << phi << "\n";
      printVec("Gradient: ", gradient);
      printVec("Normal: ", normal);
      std::cout << "Gradient norm: " << length(gradient) << "\n";
    }

  } catch (const std::exception& exc) {
    std::cerr << "AdaSDF-CL example failed: " << exc.what() << "\n";
    return 2;
  }
  return 0;
}

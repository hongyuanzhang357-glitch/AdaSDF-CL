#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 3) {
    std::cout << "Usage: adasdf_build_then_query input.stl output.sdfbin\n";
    return 0;
  }

  const std::string input = argv[1];
  const std::string output = argv[2];

  try {
    std::cout << "AdaSDF-CL build-then-query example\n";
    std::cout << "Builder backend: " << ExistingBuilderBridge::backendDescription()
              << "\n";
    std::cout << "Surrogate recommendation: "
              << (SurrogateRecommender::isAvailable() ? "available" : "unavailable")
              << "\n";

    BuildOptions options;
    options.max_octree_level = 5;
    options.max_rank = 8;
    options.verbose = false;

    auto model = AdaptiveSDFBuilder::fromMesh(input, options);
    SDFBinWriter::write(output, *model);
    auto reloaded = SDFBinReader::read(output);

    const AABB bounds = reloaded->boundingBox();
    const Vector3 center = 0.5 * (bounds.min + bounds.max);
    const std::vector<Vector3> points = {
        center,
        bounds.min,
        bounds.max,
        {center.x + 0.25 * (bounds.max.x - bounds.min.x), center.y, center.z}};

    std::cout << "Reload validation: "
              << (reloaded->isValid() ? "success" : "failed") << "\n";
    for (const Vector3& point : points) {
      printVec("Query point: ", point);
      std::cout << "Signed distance: " << reloaded->sampleDistance(point) << "\n";
      printVec("Gradient: ", reloaded->sampleGradient(point));
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_then_query failed: " << exc.what() << "\n";
    return 1;
  }
}

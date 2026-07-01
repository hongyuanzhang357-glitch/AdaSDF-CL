#include <adasdf/adasdf.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

adasdf::TriangleMesh makeCubeMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.0, 1.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0},
      {1.0, 0.0, 1.0},
      {1.0, 1.0, 1.0},
      {0.0, 1.0, 1.0}};
  mesh.triangles = {
      {0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
      {0, 1, 5}, {0, 5, 4}, {1, 2, 6}, {1, 6, 5},
      {2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}};
  return mesh;
}

}  // namespace

int main() {
  try {
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() / "adasdf_v1_6_compare_demo";
    std::filesystem::create_directories(temp);
    const std::filesystem::path stl_path = temp / "cube_generated.stl";
    std::string write_error;
    if (!adasdf::STLWriter::write(stl_path.string(), makeCubeMesh(), {}, &write_error)) {
      std::cerr << "STL write failed: " << write_error << "\n";
      return 1;
    }

    adasdf::DenseSDFBuildOptions dense_options;
    dense_options.resolution = 18;
    dense_options.padding = 0.05;
    adasdf::DenseSDFBuildReport dense_report;
    auto dense = adasdf::DenseSDFBuilder::fromSTL(
        stl_path.string(),
        dense_options,
        &dense_report);

    adasdf::AdaptiveBlockSDFBuildOptions adaptive_options;
    adaptive_options.max_octree_level = 3;
    adaptive_options.block_resolution = 6;
    adasdf::AdaptiveBlockSDFBuildReport adaptive_report;
    auto adaptive = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        stl_path.string(),
        adaptive_options,
        &adaptive_report);

    if (!dense || !adaptive) {
      std::cerr << "Dense or adaptive build failed\n";
      return 1;
    }

    const std::vector<adasdf::Vector3> points = {
        {0.5, 0.5, 0.5},
        {1.1, 0.5, 0.5},
        {0.5, 0.1, 0.5},
        {0.5, 0.5, 1.05}};
    double max_abs_diff = 0.0;
    for (const adasdf::Vector3& p : points) {
      const double a = dense->sampleDistance(p);
      const double b = adaptive->sampleDistance(p);
      max_abs_diff = std::max(max_abs_diff, std::abs(a - b));
    }

    std::cout << "AdaSDF-CL AdaptiveBlockSDF vs DenseSDF demo\n";
    std::cout << "Dense memory bytes: " << dense->memoryFootprintBytes() << "\n";
    std::cout << "Adaptive memory bytes: "
              << adaptive->memoryFootprintBytes() << "\n";
    std::cout << "Adaptive blocks: " << adaptive_report.block_count << "\n";
    std::cout << "Max abs phi difference: " << max_abs_diff << "\n";
    std::cout
        << "Note: v1.6 adaptive blocks store dense phi values; low-rank "
           "compression is planned for v1.7.0-alpha.\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_adaptive_vs_dense_sdf_demo failed: " << exc.what()
              << "\n";
    return 1;
  }
}

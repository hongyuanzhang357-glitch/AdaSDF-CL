#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

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
        std::filesystem::temp_directory_path() / "adasdf_v1_6_adaptive_demo";
    std::filesystem::create_directories(temp);
    const std::filesystem::path stl_path = temp / "cube_generated.stl";
    const std::filesystem::path sdf_path = temp / "cube_adaptive_block.sdfbin";

    std::string write_error;
    if (!adasdf::STLWriter::write(stl_path.string(), makeCubeMesh(), {}, &write_error)) {
      std::cerr << "STL write failed: " << write_error << "\n";
      return 1;
    }

    adasdf::AdaptiveBlockSDFBuildOptions options;
    options.max_octree_level = 3;
    options.block_resolution = 6;
    adasdf::AdaptiveBlockSDFBuildReport report;
    auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        stl_path.string(),
        options,
        &report);
    if (!model) {
      std::cerr << "AdaptiveBlockSDF build failed: "
                << report.error_message << "\n";
      return 1;
    }
    adasdf::SDFBinWriter::write(sdf_path.string(), *model);
    auto reloaded = adasdf::SDFBinReader::read(sdf_path);

    const double center_phi = reloaded->sampleDistance({0.5, 0.5, 0.5});
    const double outside_phi = reloaded->sampleDistance({1.5, 0.5, 0.5});

    adasdf::CollisionObject a(reloaded);
    adasdf::CollisionObject b(reloaded);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));
    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    adasdf::CollisionResult collision;
    adasdf::collide(a, b, request, collision);

    std::cout << "AdaSDF-CL STL to AdaptiveBlockSDF demo\n";
    std::cout << "Generated STL: " << stl_path.string() << "\n";
    std::cout << "Output: " << sdf_path.string() << "\n";
    std::cout << "Octree nodes: " << report.octree_node_count << "\n";
    std::cout << "Leaf blocks: " << report.block_count << "\n";
    std::cout << "Near-surface blocks: "
              << report.near_surface_block_count << "\n";
    std::cout << "Center phi: " << center_phi << "\n";
    std::cout << "Outside phi: " << outside_phi << "\n";
    std::cout << "Collision contacts: " << collision.contacts().size() << "\n";
    if (!collision.contacts().empty()) {
      const adasdf::Contact& contact = collision.contacts().front();
      std::cout << "Contact point: " << contact.point << "\n";
      std::cout << "Contact normal: " << contact.normal << "\n";
      std::cout << "Penetration depth: " << contact.penetration_depth << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_stl_to_adaptive_block_sdf_demo failed: "
              << exc.what() << "\n";
    return 1;
  }
}

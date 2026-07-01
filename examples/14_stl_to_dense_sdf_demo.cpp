#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      std::cout << "Usage: adasdf_stl_to_dense_sdf_demo input.stl [output.sdfbin]\n";
      return 0;
    }
    const std::filesystem::path input = argv[1];
    const std::filesystem::path output =
        argc >= 3 ? std::filesystem::path(argv[2])
                  : (std::filesystem::temp_directory_path() / "adasdf_dense_demo.sdfbin");

    adasdf::DenseSDFBuildOptions options;
    options.resolution = 24;
    options.padding = 0.05;
    adasdf::DenseSDFBuildReport report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(input.string(), options, &report);
    if (!model) {
      std::cerr << "DenseSDF build failed: " << report.error_message << "\n";
      return 1;
    }
    adasdf::SDFBinWriter::write(output.string(), *model);
    auto reloaded = adasdf::SDFBinReader::read(output);

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

    std::cout << "AdaSDF-CL STL to DenseSDF demo\n";
    std::cout << "Output: " << output.string() << "\n";
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
    std::cerr << "adasdf_stl_to_dense_sdf_demo failed: " << exc.what() << "\n";
    return 1;
  }
}

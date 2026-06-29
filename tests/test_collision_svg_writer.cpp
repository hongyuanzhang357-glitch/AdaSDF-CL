#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  try {
    adasdf::DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = true;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

    adasdf::CollisionObject a(build.model);
    adasdf::CollisionObject b(build.model);
    b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

    adasdf::CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;

    adasdf::CollisionResult result;
    adasdf::collide(a, b, request, result);

    std::filesystem::path dir = ADASDF_CL_TEST_TEMP_DIR;
    if (dir.empty()) {
      dir = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(dir);
    const std::filesystem::path svg_path = dir / "collision_view.svg";

    adasdf::CollisionSVGScene scene;
    scene.box_a = a.getAABB();
    scene.box_b = b.getAABB();
    scene.result = result;
    scene.requested_max_contacts = request.max_contacts;
    adasdf::CollisionSVGWriter::write(svg_path, scene);

    std::ifstream input(svg_path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string text = buffer.str();
    if (!std::filesystem::exists(svg_path) ||
        text.find("<svg") == std::string::npos ||
        text.find("contact-marker") == std::string::npos) {
      std::cerr << "collision SVG is missing expected content\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_collision_svg_writer failed: "
              << exc.what() << "\n";
    return 1;
  }
}

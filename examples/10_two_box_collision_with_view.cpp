#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

int main() {
  using namespace adasdf;

  try {
    DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = true;
    build_request.target_near_surface_error = 1.0e-3;
    build_request.memory_limit_mb = 64.0;
    build_request.block_expand_limit_mb = 16.0;

    const auto build = DemoAdaptiveSDFBuilder::build(build_request);

    CollisionObject object_a(build.model);
    CollisionObject object_b(build.model);
    object_b.setTransform(Transform::fromTranslation({0.25, 0.0, 0.0}));

    CollisionRequest request;
    request.enable_contact = true;
    request.enable_gradient = true;
    request.max_contacts = 8;
    request.query_mode = QueryMode::Balanced;

    CollisionResult collision;
    const bool hit = collide(object_a, object_b, request, collision);

    CollisionSVGScene scene;
    scene.box_a = object_a.getAABB();
    scene.box_b = object_b.getAABB();
    scene.result = collision;
    scene.title = "AdaSDF-CL v0.9 example collision";
    scene.requested_max_contacts = request.max_contacts;

    const auto svg_path =
        std::filesystem::temp_directory_path() /
        "adasdf_v0_9_two_box_collision.svg";
    CollisionSVGWriter::write(svg_path, scene);

    std::cout << "AdaSDF-CL two-box collision with view\n";
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Requested max contacts: " << request.max_contacts << "\n";
    std::cout << "Returned contacts: " << collision.contacts().size() << "\n";
    std::cout << "SVG view: " << svg_path.string() << "\n";
    return hit &&
        collision.contacts().size() <= static_cast<std::size_t>(request.max_contacts) &&
        std::filesystem::exists(svg_path) ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_two_box_collision_with_view failed: "
              << exc.what() << "\n";
    return 1;
  }
}

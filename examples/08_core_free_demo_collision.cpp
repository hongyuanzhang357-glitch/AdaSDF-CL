#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

namespace {

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main() {
  using namespace adasdf;

  try {
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "adasdf_core_free_demo_box.sdfbin";

    auto model = AnalyticSDFModel::createBox();
    SDFBinWriter::write(output.string(), *model);
    auto reloaded = SDFBinReader::read(output);

    CollisionObject object_a(reloaded);
    CollisionObject object_b(reloaded);
    object_b.setTransform(Transform::fromTranslation({0.25, 0.0, 0.0}));

    CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 4;
    request.contact_tolerance = 1.0e-4;
    request.query_mode = QueryMode::Balanced;

    CollisionResult collision;
    const bool hit = collide(object_a, object_b, request, collision);

    DistanceRequest distance_request;
    DistanceResult distance_result;
    const Scalar d = distance(object_a, object_b, distance_request, distance_result);

    std::cout << "AdaSDF-CL core-free demo collision\n";
    std::cout << "Demo sdfbin: " << output.string() << "\n";
    std::cout << "Backend: " << reloaded->metadata().query_backend << "\n";
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << collision.minimumDistance() << "\n";
    std::cout << "Distance query: " << d << "\n";
    std::cout << "Contact count: " << collision.contacts().size() << "\n";
    if (!collision.contacts().empty()) {
      const Contact& contact = collision.contacts().front();
      printVec("Contact[0].point: ", contact.point);
      printVec("Contact[0].normal: ", contact.normal);
      std::cout << "Contact[0].penetration_depth: "
                << contact.penetration_depth << "\n";
    }
    std::cout << "Method: " << collision.methodInfo() << "\n";
    return hit && !collision.contacts().empty() && std::isfinite(d) ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_core_free_demo_collision failed: " << exc.what() << "\n";
    return 1;
  }
}

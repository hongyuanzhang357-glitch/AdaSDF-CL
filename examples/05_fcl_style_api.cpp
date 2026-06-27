#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 2) {
    std::cout << "Usage: adasdf_fcl_style_api path/to/model.sdfbin\n";
    return 0;
  }

  try {
    auto model = SDFBinReader::read(std::filesystem::path(argv[1]));

    FCLStyleCollisionObject obj_a{CollisionObject(model)};
    FCLStyleCollisionObject obj_b{CollisionObject(model)};
    obj_a.object().setTransform(Transform::Identity());
    obj_b.object().setTransform(Transform::fromTranslation({0.01, 0.0, 0.0}));

    CollisionRequest request;
    request.enable_contact = true;
    request.max_contacts = 16;
    request.contact_tolerance = 1.0e-4;
    request.backend = BackendType::CPU;

    CollisionResult result;
    const bool hit = FCLAdapter::collide(obj_a, obj_b, request, result);

    std::cout << "AdaSDF-CL FCL-style API example\n";
    std::cout << "Backend: " << result.backendInfo() << "\n";
    std::cout << "Method: " << result.methodInfo() << "\n";
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << result.minimumDistance() << "\n";
    std::cout << "Candidate points: " << result.numCandidatePoints() << "\n";
    std::cout << "Raw contacts: " << result.numRawContacts() << "\n";
    std::cout << "Reduced contacts: " << result.numReducedContacts() << "\n";
    std::cout << "Contacts: " << result.contacts().size() << "\n";
    std::cout << "Number of SDF queries: " << result.numSDFQueries() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "AdaSDF-CL FCL-style API example failed: " << exc.what() << "\n";
    return 1;
  }
}

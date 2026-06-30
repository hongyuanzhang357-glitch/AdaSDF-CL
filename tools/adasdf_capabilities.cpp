#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void usage() {
  std::cout << "Usage: adasdf_capabilities [--verbose]\n";
}

void printList(const char* title, const char* const* items, std::size_t count) {
  std::cout << "\n" << title << ":\n";
  for (std::size_t i = 0; i < count; ++i) {
    std::cout << "- " << items[i] << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--verbose") {
        verbose = true;
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown option: " << arg << "\n";
        usage();
        return 2;
      }
    }

    static const char* const implemented[] = {
        "core-free demo adaptive SDF",
        "FCL-style collision API",
        "point signed-distance, gradient, and normal query",
        "CPU direct/global/block query",
        "CUDA global/block expanded query when CUDA is enabled",
        "expansion quality audit",
        "sign mismatch and near-surface mismatch metrics",
        "SVG collision view",
        "CMake find_package integration"};
    static const char* const partial[] = {
        "demo surrogate recommender",
        "contact reduction and research-preview contact output",
        "existing-core sampled expansion bridge",
        "CUDA expanded query backend",
        "block-expanded query for local contact regions"};
    static const char* const planned[] = {
        "standalone arbitrary STL builder",
        "mesh diagnostics and repair",
        "FCL fallback backend",
        "CollisionWorld broadphase",
        "CCD",
        "Python bindings",
        "ROS / MoveIt integration",
        "full low-rank GPU-native SDF query"};

    std::cout << "AdaSDF-CL version: " << adasdf::versionString() << "\n";
    std::cout
        << "Position: FCL-style SDF collision backend under development.\n";
    std::cout
        << "Boundary: complementary SDF backend, not a drop-in FCL replacement.\n";

    printList("Implemented", implemented, std::size(implemented));
    printList("Experimental / partial", partial, std::size(partial));
    printList("Planned", planned, std::size(planned));

    if (verbose) {
      std::cout << "\nQuery backend matrix:\n";
      std::cout << "- CPU + none: direct model phi and normal query\n";
      std::cout << "- CPU + global/block: pre-expanded dense query\n";
      std::cout << "- CUDA + global/block: resident expanded query when enabled\n";
      std::cout << "- CUDA + none: not supported; CUDA requires expanded data\n";
      std::cout << "\nContact output:\n";
      std::cout << "- contact point, normal, penetration depth, signed distance\n";
      std::cout << "- raw/reduced contact counts and max contact cap\n";
      std::cout << "- stable robot-grade manifold clustering and CCD are planned\n";
      std::cout << "\nDocumentation:\n";
      std::cout << "- docs/capability_matrix.md\n";
      std::cout << "- docs/implemented_vs_planned.md\n";
      std::cout << "- docs/fcl_complement_strategy.md\n";
      std::cout << "- docs/query_backend_matrix.md\n";
      std::cout << "- docs/contact_output_matrix.md\n";
      std::cout << "- docs/public_positioning.md\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_capabilities failed: " << exc.what() << "\n";
    return 1;
  }
}

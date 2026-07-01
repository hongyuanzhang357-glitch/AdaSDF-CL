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
        "ASCII and binary STL reader for mesh diagnostics",
        "ASCII STL writer for cleaned mesh export",
        "STL mesh diagnostics preflight report",
        "SDF build readiness scoring and repair suggestions",
        "safe mesh cleanup and before/after cleanup reports",
        "standalone uniform DenseSDF builder",
        "ADASDF_DENSE_SDFBIN_V1 read/write",
        "STL-to-DenseSDF public workflow",
        "expansion quality audit",
        "sign mismatch and near-surface mismatch metrics",
        "SVG collision view",
        "CMake find_package integration"};
    static const char* const partial[] = {
        "demo surrogate recommender",
        "contact reduction and research-preview contact output",
        "adaptive builder interface preview",
        "adasdf_build_adaptive_sdf_preview dry-run",
        "existing-core sampled expansion bridge",
        "CUDA expanded query backend",
        "block-expanded query for local contact regions"};
    static const char* const planned[] = {
        "standalone adaptive compressed STL builder",
        "adaptive octree/block builder",
        "low-rank compression builder",
        "surrogate-guided recommendation",
        "complex mesh repair and hole filling",
        "self-intersection detection",
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
      std::cout << "\nMesh diagnostics:\n";
      std::cout << "- ASCII / binary STL reader for diagnostics\n";
      std::cout << "- watertight, boundary, non-manifold, duplicate, degenerate, component, and scale checks\n";
      std::cout << "- readiness score, severity classification, and repair suggestions\n";
      std::cout << "- safe cleanup removes near-duplicate vertices, degenerate triangles, duplicate triangles, and unused vertices\n";
      std::cout << "- cleaned ASCII STL export through STLWriter and adasdf_mesh_clean\n";
      std::cout << "- public uniform DenseSDF build through adasdf_build_dense_sdf\n";
      std::cout << "- adaptive compressed builder preview only; no octree/block/low-rank build in v1.5\n";
      std::cout << "- no hole filling, self-intersection repair, or adaptive compressed STL-to-SDF builder\n";
      std::cout << "\nDocumentation:\n";
      std::cout << "- docs/capability_matrix.md\n";
      std::cout << "- docs/mesh_diagnostics.md\n";
      std::cout << "- docs/mesh_readiness.md\n";
      std::cout << "- docs/mesh_cleanup.md\n";
      std::cout << "- docs/dense_sdf_builder.md\n";
      std::cout << "- docs/stl_to_sdf_public_workflow.md\n";
      std::cout << "- docs/adaptive_builder_preview.md\n";
      std::cout << "- docs/stl_import_audit.md\n";
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

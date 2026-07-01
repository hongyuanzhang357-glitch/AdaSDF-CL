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
        "adaptive octree builder",
        "adaptive block partitioner",
        "adaptive block SDF builder",
        "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1 read/write",
        "adasdf_build_adaptive_sdf",
        "block low-rank compression",
        "matrix-SVD block compression",
        "compressed adaptive block SDF model",
        "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1 read/write",
        "adasdf_compress_adaptive_sdf",
        "adasdf_build_compressed_sdf",
        "surrogate-guided build recommendation",
        "deterministic build surrogate estimator",
        "mesh-feature-based parameter recommendation",
        "adasdf_recommend_build",
        "expansion quality audit",
        "sign mismatch and near-surface mismatch metrics",
        "SVG collision view",
        "CMake find_package integration"};
    static const char* const partial[] = {
        "demo surrogate recommender",
        "contact reduction and research-preview contact output",
        "adaptive builder preview / planning tool",
        "adaptive refinement heuristic",
        "block-wise dense storage",
        "error-bounded rank selection",
        "memory-bounded rank selection",
        "memory/error build estimator",
        "recommendation confidence scoring",
        "optional surrogate profile",
        "compressed direct query",
        "dense fallback blocks",
        "brute-force adaptive block sampling",
        "adasdf_build_adaptive_sdf_preview dry-run",
        "existing-core sampled expansion bridge",
        "CUDA expanded query backend",
        "block-expanded query for local contact regions"};
    static const char* const planned[] = {
        "Tucker/HOSVD compression",
        "trained surrogate model integration",
        "online recommendation calibration",
        "complex mesh repair and hole filling",
        "self-intersection detection",
        "FCL fallback backend",
        "CollisionWorld broadphase",
        "CCD",
        "Python bindings",
        "ROS / MoveIt integration",
        "full low-rank GPU-native SDF query",
        "GPU-native compressed query"};

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
      std::cout << "- public adaptive block build through adasdf_build_adaptive_sdf\n";
      std::cout << "- public compressed adaptive block build through adasdf_build_compressed_sdf\n";
      std::cout << "- matrix-SVD block compression with dense fallback in v1.7\n";
      std::cout << "- surrogate-guided build recommendation through adasdf_recommend_build in v1.8\n";
      std::cout << "- v1.8 recommendation is deterministic and experimental; it is not a universal trained model, not fully trained, and not an optimality guarantee\n";
      std::cout << "- no hole filling, self-intersection repair, Tucker/HOSVD, trained model integration, online calibration, or GPU-native compressed query\n";
      std::cout << "\nDocumentation:\n";
      std::cout << "- docs/capability_matrix.md\n";
      std::cout << "- docs/mesh_diagnostics.md\n";
      std::cout << "- docs/mesh_readiness.md\n";
      std::cout << "- docs/mesh_cleanup.md\n";
      std::cout << "- docs/dense_sdf_builder.md\n";
      std::cout << "- docs/stl_to_sdf_public_workflow.md\n";
      std::cout << "- docs/adaptive_builder_preview.md\n";
      std::cout << "- docs/stl_import_audit.md\n";
      std::cout << "- docs/low_rank_block_compression.md\n";
      std::cout << "- docs/compressed_block_sdfbin_format.md\n";
      std::cout << "- docs/stl_to_compressed_sdf_workflow.md\n";
      std::cout << "- docs/surrogate_guided_recommendation.md\n";
      std::cout << "- docs/recommended_build_workflow.md\n";
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

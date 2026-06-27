# AdaSDF-CL Audit Report

## Scope

This report reviews the existing SDF / collision project and records the first-pass organization work for a future open-source library named AdaSDF-CL.

The current project root is an active research workspace rather than a clean library root. To avoid breaking existing code, the first-pass open-source skeleton was added under `adasdf_cl/`.

## Current Project Structure Overview

- `include/sdf/`: existing C++ public/internal headers for mesh, BVH, SDF, octree, block partition, low-rank compression, model I/O, contact queries, FCL adapter, and CUDA headers.
- `src/`: matching C++ and CUDA implementation files.
- `apps/`: command-line app and Python UI application.
- `benchmarks/`: benchmark executables.
- `tests/`: C++ and Python tests for geometry, SDF, octree, compression, model I/O, contact, CUDA, UI-side helpers, and CLI behavior.
- `tools/`: utility scripts such as simple geometry STL generation.
- `data/`: sample STL data and processed geometry.
- `models/`: generated `.sdfbin`, metadata, and UI output assets.
- `reports/`: benchmark, workflow, contact, compression, and UI reports.
- `paper_package/`: paper-oriented tables and artifacts.
- `build/`: generated build trees and third-party installed files.
- `.venv-sdf-ui/`, `__pycache__`: local Python environment/cache artifacts.

## Code That Can Become Core

- Geometry and mesh base: `AABB`, `Mesh`, `StlReader`, `TriangleDistance`, `RayTriangle`, `Transform3D`, `BVH`.
- SDF generation/query: `SignedDistance`, `SDFNormal`, `OctreeSDF`, `GlobalDenseGrid`.
- Adaptive block structure: `BlockPartition`, `DenseBlockGenerator`, `DenseBlockCache`.
- Compression: `LowRankBlock`, `LowRankModel`, `LowRankReconstruction`, `MatrixSVDCompressor`, `TuckerCompressor`, `TTCompressor`, `AdaptiveBlockCompressor`, `AdaptiveLowRankModel`, `LowRankCompressionStats`.
- Model persistence: `SDFModelPackage`, `ModelIO`.
- Collision/contact query: `PairCollisionSDF`, `AllPointsSDFContact`, `StreamingBVHSDFContact`, `BVHPairFilter`, `BVHCandidateSampler`, `TrianglePairFilter`.
- Optional backends/adapters: `cuda/CudaGlobalDenseSDF`, `cuda/CudaAllPointsSDFContact`, guarded `FCLAdapter`.

## Code That Should Become Tools

- `apps/sdf_tool.cpp`: current CLI surface for mesh info, SDF probe, octree build, block build, compression, model build, query, and benchmark commands.
- `tools/generate_simple_geometry_stls.py`: geometry dataset generation utility.
- `benchmarks/bench_smoke.cpp` and `benchmarks/bench_signed_distance.cpp`: benchmark targets.
- Future command wrappers should map to `adasdf_build`, `adasdf_compress`, `adasdf_query`, and `adasdf_benchmark`.

## Code That Should Become Examples

- Minimal read/build/query flows currently described in the root `README.md`.
- Focused C++ tests such as `test_model_io.cpp`, `test_pair_collision_sdf.cpp`, `test_all_points_sdf_contact.cpp`, and CUDA contact tests can inspire examples after their setup is simplified.
- The first pass adds clean interface examples under `adasdf_cl/examples/`.

## Code Temporarily Left Unchanged

- All existing `include/sdf` and `src` algorithm files.
- Root `CMakeLists.txt`, `README.md`, `CMakePresets.json`, and `vcpkg.json`.
- Existing data, models, reports, UI, paper package, tests, and build outputs.

## Code Recommended for Later Archival or Exclusion

- `build/` generated trees and `vcpkg_installed` contents.
- `.venv-sdf-ui/` local Python virtual environment.
- `__pycache__` files under tests and UI packages.
- UI-generated benchmark CSVs and model output snapshots that are not required as reproducible examples.
- Paper-specific derived tables unless they are moved to a separate reproducibility package.

## Main Open-source Blockers

- The root is not currently a Git repository in this checkout, so provenance and ignore rules need to be established before publication.
- Generated build artifacts, virtual environments, reports, and model outputs are mixed with source files.
- Public API is still under the old `sdf` namespace and is stage/research oriented rather than a cohesive library interface.
- Root README is a development log with many stage snapshots, not a concise project landing page.
- Eigen, nlohmann_json, cxxopts, fmt, GTest, benchmark, FCL, and CUDA dependency boundaries need clearer public/private separation.
- `.sdfbin` is implemented but needs stable public documentation and compatibility policy.
- CUDA, differentiable contact, and FCL integration exist as partial or optional paths and should not be advertised as complete.

## First-pass Files Added or Modified

All first-pass files were added under `adasdf_cl/`. No existing source files were modified or moved.

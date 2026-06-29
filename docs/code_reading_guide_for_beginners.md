# Code Reading Guide for Beginners

This guide is for readers who want to understand the public AdaSDF-CL surface before changing internals.

## Start Here

- Public umbrella header: `include/adasdf/adasdf.h`
- Version metadata: `include/adasdf/version.h`
- Analytic demo model: `include/adasdf/geometry/AnalyticSDFModel.h`
- Demo `.sdfbin` reader/writer: `include/adasdf/io/DemoSDFBin.h`
- CMake package entry point: `cmake/AdaSDFCLConfig.cmake.in`
- External package smoke test: `tests/package`
- Downstream example: `examples/downstream_cmake_project`

## Backend Boundary

AdaSDF-CL currently has two practical modes:

- Core-free public build: useful for API review, CMake integration, compile/link tests, installed package smoke tests, and the v0.8 analytic demo collision workflow.
- Existing-core enhanced build: required for full adaptive STL-to-sdfbin generation and existing-core compressed `.sdfbin` assets.

Core-free builds can generate/read/query/collide demo `.sdfbin` files. The demo format is not the final adaptive compressed SDF format. Existing-core downstream consumers may need additional dependency prefixes.

## Safe Reading Order

1. Read `include/adasdf/adasdf.h` to see the public API export surface.
2. Read `include/adasdf/geometry/AnalyticSDFModel.h` and `src/geometry/AnalyticSDFModel.cpp` for the core-free backend.
3. Read `include/adasdf/io/DemoSDFBin.h` and `src/io/DemoSDFBin.cpp` for the demo file format.
4. Read `include/adasdf/query` for request/result types and `CollisionObject`.
5. Read `src/query/CpuNarrowPhase.cpp` for the approximate CPU SDF-sampling path.
6. Read `src/io/SDFBinReader.cpp` to see how demo magic is detected before existing-core fallback.
7. Read `docs/limitations.md` before interpreting collision results as guarantees.

## What Not to Assume Yet

- The demo backend is not a full adaptive compressed SDF builder.
- Pair collision is approximate and sampling based.
- FCL ABI compatibility is not provided.
- CUDA and Python integrations are not implemented.

# Code Reading Guide for Beginners

This guide is for readers who want to understand the public AdaSDF-CL surface before changing internals.

## Start Here

- Public umbrella header: `include/adasdf/adasdf.h`
- Version metadata: `include/adasdf/version.h`
- CMake package entry point: `cmake/AdaSDFCLConfig.cmake.in`
- External package smoke test: `tests/package`
- Downstream example: `examples/downstream_cmake_project`

## Backend Boundary

AdaSDF-CL currently has two practical modes:

- Core-free public build: useful for API review, CMake integration, compile/link tests, and installed package smoke tests.
- Existing-core enhanced build: required for real `.sdfbin` generation, reading, point query, and pair collision over existing-core models.

Core-free builds currently cannot generate/read real `.sdfbin` models. Real `.sdfbin` functionality currently requires existing-core enhanced builds. Existing-core downstream consumers may need additional dependency prefixes.

## Safe Reading Order

1. Read `include/adasdf/adasdf.h` to see the public API export surface.
2. Read `include/adasdf/query` for request/result types and `CollisionObject`.
3. Read `src/query/CpuNarrowPhase.cpp` for the approximate CPU SDF-sampling path.
4. Read `include/adasdf/io/SDFBinReader.h` and `src/io/SDFBinReader.cpp` to understand why real `.sdfbin` reading needs the existing core today.
5. Read `docs/limitations.md` before interpreting collision results as guarantees.

## What Not to Assume Yet

- The public core-free build is not a clone-only end-to-end collision demo.
- Pair collision is approximate and sampling based.
- FCL ABI compatibility is not provided.
- CUDA and Python integrations are not implemented.

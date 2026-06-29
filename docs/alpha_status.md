# Alpha Status

AdaSDF-CL 0.7.0-alpha.2 is a research-preview release candidate.

## What Works in 0.7.0-alpha.2

- STL cube build through the adaptive builder bridge when the existing core is available.
- `.sdfbin` reader/writer round-trip for existing-core-backed models.
- Point signed-distance query.
- Finite-difference gradient query.
- FCL-style object/request/result interface.
- CPU approximate SDF-sampling narrow-phase.
- Symmetric contact generation.
- Deterministic contact reduction.
- Installable CMake package with `AdaSDFCL::adasdf_cl`.
- Package config smoke test and standalone downstream CMake example.
- Installed `adasdf_build`, `adasdf_info`, `adasdf_query`, and `adasdf_collide` tools.
- Example and test builds without CUDA, FCL, Python, UI, or large datasets.
- Repository clean check.

## Current Backend Boundary

The core-free public build supports configure/build/test/install, installed CLI
tools, and downstream `find_package(AdaSDFCL CONFIG REQUIRED)` integration. It
does not yet generate, read, or query real `.sdfbin` models by itself.

Real `.sdfbin` generation, reading, point query, and pair collision currently
require an existing-core enhanced build with `ADASDF_CL_USE_EXISTING_CORE=ON`
and a valid `ADASDF_CL_EXISTING_ROOT`.

Existing-core downstream consumers may also need additional dependency prefixes,
such as the existing core's vcpkg-installed dependencies, in `CMAKE_PREFIX_PATH`.

## What Is Experimental

- DASH-SDF surrogate recommendation API.
- Contact-only `.sdfbin`.
- Adaptive builder bridge robustness beyond small validation meshes.
- Distance query global optimality.
- Pair collision accuracy over complex nonconvex geometries.
- CPack source archive generation.

## What Is Not Implemented Yet

- CUDA batched pair collision.
- FCL ABI-compatible adapter.
- Python package.
- Industrial certified contact solver.
- Universal surrogate guarantee.
- Strict global closest-distance solver.
- Standalone public backend for clone-only `.sdfbin` generation/read/query.

## Validation Snapshot

- Expected tests in the alpha validation build: 17.
- Expected install validation: PASS when run with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected clean check: PASS.
- External collision test verdict for v0.7.0-alpha.1: PARTIAL.
- Core-free external CMake compile/link smoke test: PASS.
- Existing-core build, query, and collision path: PASS in the author's local environment.

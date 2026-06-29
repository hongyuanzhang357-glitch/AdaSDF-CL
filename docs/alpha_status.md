# Alpha Status

AdaSDF-CL 0.8.0-alpha is a research-preview release candidate.

## What Works in 0.8.0-alpha

- Core-free analytic box SDF model.
- Core-free demo `.sdfbin` reader/writer using `ADASDF_DEMO_SDFBIN_V1`.
- `adasdf_make_demo_box` demo asset generator.
- `adasdf_info`, `adasdf_query`, and `adasdf_collide` over demo `.sdfbin` files.
- Core-free collision example `adasdf_core_free_demo_collision`.
- Downstream `find_package(AdaSDFCL CONFIG REQUIRED)` integration with a no-input demo collision path.
- STL cube build through the adaptive builder bridge when the existing core is available.
- Existing-core `.sdfbin` reader/writer round-trip for existing-core-backed models.
- FCL-style object/request/result interface.
- CPU approximate SDF-sampling narrow-phase.
- Symmetric contact generation and deterministic contact reduction.
- Installable CMake package with `AdaSDFCL::adasdf_cl`.
- Example and test builds without CUDA, FCL, Python, UI, or large datasets.
- Repository clean check.

## Current Backend Boundary

The core-free public build now supports a clone-only end-to-end demo:

```text
adasdf_make_demo_box -> adasdf_info -> adasdf_query -> adasdf_collide
```

This path uses an analytic box backend and a lightweight demo `.sdfbin` format. It is intended for public demos, downstream integration tests, and API learning.

Full adaptive compressed SDF generation from STL still requires an existing-core enhanced build with `ADASDF_CL_USE_EXISTING_CORE=ON` and a valid `ADASDF_CL_EXISTING_ROOT`, or future standalone builder work.

Existing-core downstream consumers may also need additional dependency prefixes, such as the existing core's vcpkg-installed dependencies, in `CMAKE_PREFIX_PATH`.

## What Is Experimental

- Core-free demo `.sdfbin` format.
- DASH-SDF surrogate recommendation API.
- Contact-only `.sdfbin`.
- Adaptive builder bridge robustness beyond small validation meshes.
- Distance query global optimality.
- Pair collision accuracy over complex nonconvex geometries.
- CPack source archive generation.

## What Is Not Implemented Yet

- Public standalone adaptive STL-to-sdfbin builder.
- Arbitrary mesh support in the core-free analytic demo backend.
- CUDA batched pair collision.
- FCL ABI-compatible adapter.
- Python package.
- Industrial certified contact solver.
- Universal surrogate guarantee.
- Strict global closest-distance solver.

## Validation Snapshot

- Expected tests in the alpha validation build: 21.
- Expected install validation: PASS when run with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected core-free demo CLI workflow: PASS.
- Expected clean check: PASS.
- External collision test verdict for v0.7.0-alpha.1: PARTIAL.
- Target external collision test verdict for v0.8.0-alpha: PASS for the demo backend.
- Existing-core build, query, and collision path: PASS in the author's local environment.

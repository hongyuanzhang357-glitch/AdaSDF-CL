# Alpha Status

AdaSDF-CL 0.7.0-alpha is a research-preview release candidate.

## What Works in 0.7.0-alpha

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

## Validation Snapshot

- Expected tests in the alpha validation build: 17.
- Expected install validation: PASS when run with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected clean check: PASS.
- Expected example path: build cube `.sdfbin`, load/query it, run pair collision, run contact reduction demo.

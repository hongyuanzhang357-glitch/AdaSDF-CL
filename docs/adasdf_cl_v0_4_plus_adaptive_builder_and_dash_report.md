# AdaSDF-CL v0.4+ Adaptive Builder and DASH-SDF Report

## Summary

AdaSDF-CL v0.4+ connects the first real adaptive builder bridge:

- Existing `.sdfbin` load/query remains active through `SDFBinReader::read()`.
- Single-model SDF queries continue through `sdf::AdaptiveLowRankModel::queryPhi()`.
- Two-object `collide()` and `distance()` keep the v0.3 CPU API-validation fallback.
- STL adaptive construction now works through the existing core.
- `.sdfbin` writing now works for existing-core-backed models and validates by reload.
- DASH-SDF recommendation is represented by a conservative placeholder API.

## Main Changes

- Added `ExistingBuilderBridge` to keep `sdf::` implementation types out of public AdaSDF-CL headers.
- Expanded `BuildOptions` with accuracy, memory, adaptive, DASH-SDF, and existing-core bridge fields.
- Implemented `AdaptiveSDFBuilder::fromMesh()` for STL files.
- Implemented `SDFBinWriter::write()` for native existing-core handles.
- Added `SurrogateRecommender` with a non-credible unavailable-backend result.
- Added the `adasdf_build` CLI.
- Added `examples/06_build_then_query.cpp`.
- Added a project-generated closed cube STL fixture.
- Added tests for build options, adaptive builder, writer round-trip, and surrogate behavior.
- Added repository hygiene files and a cleanup checker.

## Validation Commands

```bash
cmake -S adasdf_cl -B build/adasdf_cl-v0_4_plus -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=ON -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
cmake --build build/adasdf_cl-v0_4_plus --config Debug
ctest --test-dir build/adasdf_cl-v0_4_plus -C Debug --output-on-failure
build/adasdf_cl-v0_4_plus/Debug/adasdf_build.exe adasdf_cl/tests/data/cube_closed_ascii.stl build/cube_v0_4_plus.sdfbin --near-surface-error 1e-4 --max-memory-mb 256 --compress
build/adasdf_cl-v0_4_plus/Debug/adasdf_load_sdfbin_and_query.exe build/cube_v0_4_plus.sdfbin
build/adasdf_cl-v0_4_plus/Debug/adasdf_collision_between_two_objects.exe build/cube_v0_4_plus.sdfbin
python adasdf_cl/scripts/check_repo_clean.py adasdf_cl
```

## Observed Results

- CTest: 13/13 tests passed.
- `adasdf_build`: success.
- Generated cube `.sdfbin` memory footprint: 10432 bytes after reload.
- Generated cube AABB min: `-0.025 -0.025 -0.025`.
- Generated cube AABB max: `1.025 1.025 1.025`.
- Reload validation: success.
- Query example: center signed distance about `-0.499912`; offset sample signed distance about `-0.237376`.
- Pair collision example: collision true, minimum distance about `-0.489924`, 4 contacts, 154 SDF queries.
- Repo clean check: PASS.

## DASH-SDF Status

The external DASH-SDF validation card was summarized, not embedded. Key imported metrics:

- p95 prediction R2: 0.964526.
- memory prediction R2: 0.999489.
- sign-mismatch recall: 0.912281.
- false-safe rate: 0.000000.
- Top-1/Top-3/Top-5 STL-level success: 0.8 / 1.0 / 1.0.
- candidate-level success: 0.826087.
- source-card conclusion: credible within tested limits.

AdaSDF-CL runtime status:

- `SurrogateRecommender::isAvailable()` returns false.
- `recommend()` returns a non-credible unavailable-backend result.
- No surrogate model, Python runtime, joblib artifact, raw dataset, generated arrays, or large logs were migrated.

## Limitations

- STL is the only real builder input path.
- OBJ and in-memory mesh building are reserved.
- `near_surface_error` is mapped to the existing core's error-over-spacing control and is not yet an exact absolute-error optimizer.
- Memory options are exposed, but strict global memory-budget search is not implemented.
- The pair query remains an API-validation fallback.
- Contact-only `.sdfbin` remains a design target.
- CUDA, FCL adapter execution, analytic gradients, differentiable Jacobians, and Python bindings remain roadmap items.

## Next Work

- Add OBJ loader bridge or a mesh-normalization layer shared with the old core.
- Add strict memory-budget search for adaptive compression.
- Replace pair-query fallback with the real existing pair-collision bridge once native package and mesh ownership are safe.
- Define and implement contact-only `.sdfbin` conversion.
- Package DASH-SDF surrogate metadata and runtime integration behind an explicit optional build feature.

# AdaSDF-CL v0.7 External Integration Report

## Goal

AdaSDF-CL 0.7.0-alpha focuses on install-tree consumption by external CMake
projects. The target downstream contract is:

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

The release keeps CUDA, FCL, Python, and the existing OctreeBlockLowRankSDF core
optional.

## Version

- CMake project version: `0.7.0`.
- Public alpha version string: `0.7.0-alpha`.
- Citation version: `0.7.0-alpha`.
- License: MIT.

## Main Files

- `CMakeLists.txt`
- `cmake/adasdf_cl_version.cmake`
- `include/adasdf/version.h`
- `include/adasdf/config.h`
- `cmake/AdaSDFCLConfig.cmake.in`
- `tests/package/CMakeLists.txt`
- `tests/package/test_find_package.cpp`
- `tests/package/run_package_test.cmake`
- `examples/downstream_cmake_project/CMakeLists.txt`
- `examples/downstream_cmake_project/main.cpp`
- `examples/downstream_cmake_project/README.md`
- `scripts/run_install_validation.py`
- `scripts/run_alpha_validation.py`
- `scripts/check_repo_clean.py`
- `tools/adasdf_info.cpp`
- `tools/adasdf_query.cpp`
- `tools/adasdf_collide.cpp`
- `docs/external_integration.md`
- `docs/release_notes_v0_7_0_alpha.md`

## Install and Export Status

- Headers install under `include`.
- Runtime library installs under the platform library directory.
- Command-line tools install under `bin`.
- CMake config, version, and target export files install under
  `lib/cmake/AdaSDFCL`.
- `LICENSE`, `README.md`, and `CHANGELOG.md` install under the documentation
  directory.
- Installed `AdaSDFCL::adasdf_cl` resolves to the runtime static library.
- Installed `AdaSDFCL::adasdf_cl_headers` exposes the header-only interface for
  tooling and inspection.

## Downstream Test Status

`tests/package` configures with only `CMAKE_PREFIX_PATH` pointing at the install
tree. It builds and runs a small consumer executable that includes
`adasdf/adasdf.h`, links `AdaSDFCL::adasdf_cl`, and calls the runtime CPU backend.

`examples/downstream_cmake_project` is a source-tree-independent downstream
example. It links `AdaSDFCL::adasdf_cl`, prints `0.7.0-alpha`, and runs without
CUDA, FCL, Python, or the existing core.

## CLI Status

Working installed tools:

- `adasdf_build`
- `adasdf_info`
- `adasdf_query`
- `adasdf_collide`

No-argument smoke behavior:

- `adasdf_info`: prints usage and returns success.
- `adasdf_query`: prints usage and returns success.
- `adasdf_collide`: prints usage and returns success.

Existing-core cube smoke behavior:

- `adasdf_info` loads the generated cube `.sdfbin` and prints metadata.
- `adasdf_query` samples the cube at `0.5 0.5 0.5`.
- `adasdf_collide` runs a two-cube query with offset `0.01 0 0` and reports a
  colliding result with contact statistics.

## CI Summary

`.github/workflows/ci.yml` now builds outside the checkout tree, runs CTest, runs
install validation, and then runs the repository clean check. CI still uses
`ADASDF_CL_USE_EXISTING_CORE=OFF` so the external package path remains free of
existing-core, CUDA, FCL, and Python hard dependencies.

## CPack

CPack source archive metadata is configured:

- package name: `AdaSDF-CL`;
- package version: `0.7.0-alpha`;
- source generators: `ZIP` and `TGZ`;
- source ignore rules exclude generated build/install trees, logs, `.sdfbin`,
  `.npz`, package archives, caches, raw/quarantine data, and experiment outputs.

No package archive is committed.

## Validation

Manual core-free build path:

```bash
cmake -S . -B build/adasdf_cl-v0_7 -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
cmake --build build/adasdf_cl-v0_7 --config Debug
ctest --test-dir build/adasdf_cl-v0_7 -C Debug --output-on-failure
cmake --install build/adasdf_cl-v0_7 --config Debug --prefix build/adasdf_cl-v0_7-install
```

Result:

- Configure: PASS.
- Build: PASS.
- CTest: PASS, 17/17 tests.
- Install: PASS.

Install validation:

```bash
python scripts/run_install_validation.py --source . --build build/adasdf_cl-v0_7-install-validation --install build/adasdf_cl-v0_7-install --config Debug
```

Result: PASS.

Alpha validation with install validation:

```bash
python scripts/run_alpha_validation.py --source . --build ../build/adasdf_cl-v0_7-alpha-validation --config Debug --include-install
```

Result: PASS.

Repository clean check:

```bash
python scripts/check_repo_clean.py .
```

Result: PASS.

Generated source-tree `build/` and `scripts/__pycache__/` directories were
removed before final clean check and commit preparation.

## Reports

- `reports/install_validation_summary.md`
- `reports/alpha_validation_summary.md`

Both reports use placeholder paths instead of local absolute paths.

## Git

- Local commit message planned: `Prepare AdaSDF-CL 0.7.0-alpha external integration`.
- Commit hash: recorded in the final handoff after the commit is created. A
  commit cannot embed its own final hash in a tracked file without changing that
  hash.
- Remote: no remote configured; no push performed.

## Limitations

- Existing `.sdfbin` point sampling still requires an optional existing-core
  build.
- Pair collision remains an approximate SDF-sampling narrow-phase.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not implemented.
- Python bindings are not implemented.
- The DASH-SDF surrogate recommender remains a placeholder unless a future
  backend is connected.
- Contact-only `.sdfbin` remains a design/format surface, not a full production
  writer/reader path.

## Next Round

- Add Python binding preview and package layout.
- Decide whether source archives should be generated in CI as artifacts.
- Add a shorter Windows path strategy for package smoke tests in release jobs.
- Extend CLI regression tests around `.sdfbin` fixtures when the existing core is
  available.
- Start CUDA batched query design only after CPU/package surfaces remain stable.

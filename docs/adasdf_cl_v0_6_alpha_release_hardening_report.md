# AdaSDF-CL v0.6 Alpha Release Hardening Report

## Goal

Prepare `adasdf_cl` as a clean local alpha / research-preview repository candidate for future GitHub publication. This round focused on versioning, public documentation, release notes, CMake install/export support, CI templates, validation scripts, repository hygiene, and local git initialization.

This round did not add CUDA, Python bindings, a real FCL ABI adapter, GPU pair collision, industrial benchmarks, or major algorithm rewrites.

## Completed Work

- Added centralized version metadata for `0.6.0-alpha`.
- Reworked the public README for GitHub readers.
- Added changelog and v0.6 alpha release notes.
- Added contribution and code of conduct documents.
- Added GitHub CI, issue, and pull request templates.
- Added CMake package config skeleton and install/export support.
- Added examples, tools, and tests README files.
- Added alpha status documentation.
- Added alpha validation script and generated a compact validation report.
- Strengthened repository clean checks.
- Preserved MIT licensing.
- Prepared the subtree for local git initialization and initial commit.

## New Files

- `include/adasdf/version.h`
- `cmake/adasdf_cl_version.cmake`
- `cmake/AdaSDFCLConfig.cmake.in`
- `CHANGELOG.md`
- `CONTRIBUTING.md`
- `CODE_OF_CONDUCT.md`
- `.github/workflows/ci.yml`
- `.github/ISSUE_TEMPLATE/bug_report.md`
- `.github/ISSUE_TEMPLATE/feature_request.md`
- `.github/pull_request_template.md`
- `examples/README.md`
- `tools/README.md`
- `tests/README.md`
- `docs/alpha_status.md`
- `docs/release_notes_v0_6_0_alpha.md`
- `docs/adasdf_cl_v0_6_alpha_release_hardening_report.md`
- `scripts/run_alpha_validation.py`
- `reports/alpha_validation_summary.md`

## Modified Files

- `CMakeLists.txt`
- `README.md`
- `CITATION.cff`
- `include/adasdf/adasdf.h`
- `include/adasdf/config.h`
- `docs/api_design.md`
- `docs/architecture.md`
- `docs/fcl_compatibility.md`
- `docs/limitations.md`
- `docs/roadmap.md`
- `scripts/check_repo_clean.py`

## Current Version

- CMake project version: `0.6.0`
- Public alpha version string: `0.6.0-alpha`
- Public version header: `include/adasdf/version.h`
- Citation version: `0.6.0-alpha`

## README Alpha Summary

The README now presents AdaSDF-CL as an alpha / research-preview adaptive SDF collision library. It documents:

- what works;
- what is API preview / planned;
- quick start;
- CMake options;
- STL-to-`.sdfbin` build flow;
- point query;
- FCL-style pair collision;
- current CPU approximate SDF-sampling narrow-phase;
- DASH-SDF placeholder status;
- validation snapshot;
- limitations;
- roadmap;
- citation, license, and contribution entry points.

## Release Notes Summary

`CHANGELOG.md` and `docs/release_notes_v0_6_0_alpha.md` describe this release candidate as suitable for research-preview SDF build/query/collision experiments, not for certified industrial collision pipelines. Known limitations are listed explicitly.

## CMake Install / Export Status

Implemented:

- installs headers;
- installs `adasdf_cl_runtime` static library;
- installs `adasdf_existing_core` when the existing core is built into the local alpha build;
- installs `AdaSDFCLTargets.cmake`;
- installs `AdaSDFCLConfig.cmake`;
- installs `AdaSDFCLConfigVersion.cmake`;
- exports targets under the `AdaSDFCL::` namespace.

Preview downstream usage:

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

Full downstream install-tree validation is deferred to v0.7.

## CI Configuration Summary

Added `.github/workflows/ci.yml` with:

- Windows latest build;
- Ubuntu latest build;
- examples enabled;
- tests enabled;
- `ADASDF_CL_USE_EXISTING_CORE=OFF` so CI does not depend on local parent-core paths;
- CTest;
- repository clean check.

## Examples Status

| Example | Status |
| --- | --- |
| `01_build_adaptive_sdf.cpp` | API preview |
| `02_load_sdfbin_and_query.cpp` | Working with compatible `.sdfbin` |
| `03_collision_between_two_objects.cpp` | Working with compatible `.sdfbin` |
| `04_batched_gpu_query.cpp` | API preview |
| `05_fcl_style_api.cpp` | Working FCL-style wrapper, not FCL ABI |
| `06_build_then_query.cpp` | Working when existing builder bridge is available |
| `07_contact_reduction_demo.cpp` | Working with compatible `.sdfbin` |

## Tests Status

Primary validation:

```bash
ctest --test-dir build/adasdf_cl-v0_6_alpha -C Debug --output-on-failure
```

Result: 17/17 tests passed.

CI-smoke validation with `ADASDF_CL_USE_EXISTING_CORE=OFF`:

- configure passed;
- build passed;
- 17/17 tests passed, with optional existing-core tests skipping gracefully where appropriate.

## Alpha Validation Script Result

Command:

```bash
python adasdf_cl/scripts/run_alpha_validation.py --source adasdf_cl --build build/adasdf_cl-v0_6_alpha --config Debug
```

Result: PASS.

Generated report:

- `reports/alpha_validation_summary.md`

The summary report uses placeholders for local paths and does not store machine-specific absolute paths.

## Repo Clean Check Result

Command:

```bash
python adasdf_cl/scripts/check_repo_clean.py adasdf_cl
```

Result: PASS.

The clean check now rejects local user paths, large files, `.sdfbin`, `.npz`, `.log`, generated build/cache directories, raw STL data, quarantine folders, and Python bytecode caches.

## Git Init / Commit Status

Local git initialization was completed as part of the v0.6 hardening flow:

- local repository initialized inside `adasdf_cl`;
- no remote configured;
- initial local commit created with message `Initial AdaSDF-CL 0.6.0-alpha research preview`;
- build directories and generated `.sdfbin` files excluded by `.gitignore`;
- clean check passed before commit.

## Remote Status

No GitHub remote is added in v0.6. No push is performed.

## Large File Check

No file larger than 100 MB was found by the repository clean check.

## Local Absolute Path Check

No local absolute path was found in checked `README`, docs, scripts, CMake files, or generated alpha validation report.

## Current License

MIT. The license was preserved and not changed to BSD, Apache, or another license.

## Citation Status

`CITATION.cff` is present and updated for `0.6.0-alpha`. It does not claim a DOI.

## Current Limitations

- Pair collision is an approximate CPU SDF-sampling narrow-phase.
- Distance over overlapping AABBs is approximate and sampling-dependent.
- Contact reduction is deterministic but heuristic.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- Python package is not implemented.
- DASH-SDF surrogate recommender remains a placeholder unless a future backend is connected.
- Contact-only `.sdfbin` writer/reader remains planned.
- Full downstream `find_package` validation is deferred to v0.7.

## Next Round Recommendation

AdaSDF-CL v0.7: External Integration and Packaging.

Recommended focus:

- validate `find_package(AdaSDFCL CONFIG REQUIRED)` from a standalone downstream project;
- add a minimal downstream CMake example;
- validate the install tree;
- add package config tests;
- prepare a real GitHub remote and release tag;
- refine public API naming;
- optionally prepare a Python binding preview directory without implementing bindings yet.

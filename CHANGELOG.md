# Changelog

## 0.8.0-alpha

Core-free standalone demo backend.

### Added

- Analytic box SDF model.
- Core-free demo `.sdfbin` format.
- `adasdf_make_demo_box` CLI.
- Core-free point query and collision demo.
- Demo `.sdfbin` reader/writer.
- Core-free demo collision example.
- Tests for analytic SDF, demo `.sdfbin`, make-demo-box CLI, and core-free collision.

### Changed

- README now includes a clone-only demo workflow.
- CLI documentation includes the core-free demo workflow.
- Alpha validation now runs the v0.8 make/info/query/collide demo path without the existing core.

### Notes

- This is a demo backend for public integration and learning.
- It is not a replacement for the full adaptive compressed SDF builder.
- Full adaptive STL-to-sdfbin construction remains future work for the public standalone backend.

## 0.7.0-alpha.2

Documentation hotfix after external downstream collision testing.

### Changed

- Clarified the difference between core-free public builds and existing-core enhanced builds.
- Documented that core-free builds can compile, install, and link, but cannot yet generate/read real `.sdfbin` models.
- Added external collision test summary.
- Updated README limitations for public users.

### Notes

- No core algorithm changes.
- No public API changes.
- No CMake install/export changes.

## 0.7.0-alpha.1

CI-fixed alpha / research-preview release candidate.

### Fixed

- Fixed Ubuntu install validation portability.
- Improved downstream package validation for Linux single-config generators and Windows multi-config generators.
- Preserved Windows Visual Studio compatibility.

### Notes

- No core algorithm changes.
- No public API changes.
- `v0.7.0-alpha` is retained as the initial preflight tag but should not be used as the recommended public release.

## 0.7.0-alpha

External integration and packaging alpha.

### Added

- Install-tree validation script and summary report generation.
- CMake package-config smoke test under `tests/package`.
- Standalone downstream CMake example that links `AdaSDFCL::adasdf_cl`.
- Installed command-line tools: `adasdf_info`, `adasdf_query`, and `adasdf_collide`.
- CPack source archive configuration.
- External integration documentation.
- CI install/downstream smoke validation.
- Alpha validation `--include-install` mode.

### Changed

- Installed `AdaSDFCL::adasdf_cl` now resolves to the runtime static library.
- Header-only interface is exported as `AdaSDFCL::adasdf_cl_headers`.
- Repository clean checks catch install trees, package archives, CMake generated
  files, and downstream/package build directories.

### Known Limitations

- Existing `.sdfbin` point sampling still requires a build with the optional
  existing-core bridge.
- CUDA, FCL ABI compatibility, Python bindings, and real surrogate inference
  remain out of scope for this alpha.

## 0.6.0-alpha

Initial alpha / research-preview release candidate.

### Added

- Adaptive SDF builder bridge.
- `.sdfbin` reader/writer round-trip.
- `SDFModel` point distance and gradient query.
- FCL-style `CollisionObject`, `collide`, and `distance` APIs.
- CPU SDF-sampling narrow-phase backend.
- Symmetric contact generation.
- Deterministic contact reduction.
- `adasdf_build` CLI.
- Validation card documentation.
- Repository clean check script.
- Alpha validation script.
- GitHub CI and issue/PR templates.
- CMake install/export package skeleton.

### Known Limitations

- Pair collision is an approximate SDF-sampling narrow-phase.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- Surrogate recommender is an API placeholder unless a backend is explicitly connected.
- Contact-only `.sdfbin` remains planned.

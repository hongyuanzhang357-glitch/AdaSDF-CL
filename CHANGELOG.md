# Changelog

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

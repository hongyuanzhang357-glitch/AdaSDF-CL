# Upload Package Manifest

This manifest describes what belongs in a public AdaSDF-CL source upload for the current alpha line.

## Recommended Public Tag

Use `v0.7.0-alpha.2` for the documentation-hotfix pre-release.

The earlier `v0.7.0-alpha` and `v0.7.0-alpha.1` tags are retained for traceability and should not be moved.

## Include

- Source code under `include`, `src`, `tools`, `tests`, and `examples`
- CMake packaging files under `cmake` and the root `CMakeLists.txt`
- Public docs under `docs`
- `README.md`, `CHANGELOG.md`, `LICENSE`, and `CITATION.cff`
- GitHub CI and repository metadata

## Exclude

- Build trees
- Install trees
- Generated archives
- Local experiment outputs
- Raw/quarantine datasets
- Generated `.sdfbin` files unless a future release intentionally adds a legal public fixture

## Current Backend Note

The public core-free package can configure, build, test, install, and link from external CMake projects. Core-free builds currently cannot generate/read real `.sdfbin` models. Real `.sdfbin` functionality currently requires existing-core enhanced builds.

Existing-core downstream consumers may need additional dependency prefixes, such as vcpkg-installed packages, in `CMAKE_PREFIX_PATH`.

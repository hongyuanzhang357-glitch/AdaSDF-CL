# Upload Package Manifest

This manifest describes what belongs in a public AdaSDF-CL source upload for the current alpha line.

## Recommended Public Tag

Use `v0.8.0-alpha` for the first clone-only end-to-end demo pre-release.

The earlier `v0.7.0-alpha`, `v0.7.0-alpha.1`, and `v0.7.0-alpha.2` tags are retained for traceability and should not be moved.

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
- Generated `.sdfbin` files, including demo `.sdfbin` files

## Current Backend Note

The public core-free package can configure, build, test, install, link from external CMake projects, and run a demo SDF query/collision workflow. The demo backend is analytic and box-only. Full adaptive compressed `.sdfbin` functionality currently requires existing-core enhanced builds or future standalone builder work.

Existing-core downstream consumers may need additional dependency prefixes, such as vcpkg-installed packages, in `CMAKE_PREFIX_PATH`.

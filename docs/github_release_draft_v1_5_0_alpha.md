# AdaSDF-CL v1.5.0-alpha

AdaSDF-CL v1.5.0-alpha introduces a public standalone uniform DenseSDF builder
and an experimental adaptive builder interface preview.

## Added

- `DenseSDFModel` for uniform dense signed-distance grids.
- `TriangleDistance` point-to-triangle distance utilities.
- `MeshSign` alpha-level ray-casting sign estimator for watertight meshes.
- `DenseSDFBuilder` for brute-force STL-to-uniform-DenseSDF construction.
- `DenseSDFBuildReportWriter` for Markdown and JSON-like build reports.
- `ADASDF_DENSE_SDFBIN_V1` read/write support.
- `adasdf_build_dense_sdf` CLI.
- DenseSDF compatibility with `adasdf_info`, `adasdf_query`,
  `adasdf_collide`, expansion quality, and batch benchmark `--model`.
- Adaptive builder interface preview and `adasdf_build_adaptive_sdf_preview`
  dry-run CLI.
- DenseSDF and adaptive-preview examples and tests.

## Boundaries

- DenseSDF builder is implemented and core-free.
- The dense builder creates a uniform grid, not an adaptive compressed SDF.
- Signed mode requires watertight input by default.
- Unsigned mode supports open meshes.
- The adaptive compressed builder is interface preview only in v1.5.0-alpha.
- Full octree/block/low-rank construction remains planned work.
- Surrogate-guided parameter recommendation remains planned for a later
  release.

## Validation

Expected validation for this release:

- CPU-only build: PASS.
- CTest: PASS.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean repository check: PASS.

No `.sdfbin`, `.csv`, `.svg`, `.log`, build, install, or external STL assets
are included in the source release.

# AdaSDF-CL v1.3.0-alpha Mesh Readiness Report

## Goal

Add SDF build readiness assessment on top of STL diagnostics. This release
classifies mesh issues by severity, computes a simple readiness score, and
recommends preprocessing steps before adaptive SDF construction.

## Added Files

- `include/adasdf/mesh/MeshReadiness.h`
- `src/mesh/MeshReadiness.cpp`
- `tests/test_mesh_readiness.cpp`
- `tests/test_mesh_check_readiness_cli.cpp`
- `docs/mesh_readiness.md`
- `docs/github_release_draft_v1_3_0_alpha.md`
- `docs/mesh_readiness_v1_3_report.md`

## Modified Files

- `tools/adasdf_mesh_check.cpp`
- `include/adasdf/mesh/MeshDiagnosticsWriter.h`
- `src/mesh/MeshDiagnosticsWriter.cpp`
- `include/adasdf/adasdf.h`
- `CMakeLists.txt`
- validation scripts
- README, changelog, status, roadmap, and capability docs

## Readiness Score Rules

- 100: clean watertight mesh.
- 80-99: usable with minor warnings.
- 50-79: poor, needs preprocessing.
- 0-49: not recommended for SDF build.

## Severity Rules

Critical issues include empty meshes, NaN / Inf data, invalid scale,
non-manifold edges, boundary edges when watertight input is required, and
degenerate triangles when they are not allowed.

Warnings include duplicate triangles, multiple connected components, isolated
vertices, scale warnings, and boundary edges in lenient/open mode.

Info issues provide AABB, component, and unit-confirmation context.

## CLI Usage

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_check model.stl --readiness --json mesh_report.json
adasdf_mesh_check model.stl --readiness --strict
adasdf_mesh_check model.stl --readiness --lenient
```

## Fixtures

All STL fixtures are project-generated simple geometry:

- `closed_cube_ascii.stl`
- `open_cube_missing_face_ascii.stl`
- `degenerate_triangle_ascii.stl`
- `duplicate_triangle_ascii.stl`
- `non_manifold_edge_ascii.stl`
- `two_components_ascii.stl`

## Validation Results

- CPU-only Release CTest: 61/61 PASS.
- CUDA Release CTest through an ASCII-only source/build path with CUDA v12.6:
  61/61 PASS.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.

## Current Limits

v1.3.0-alpha does not repair input STL files, does not perform full
self-intersection detection, and does not implement a standalone arbitrary-STL
adaptive SDF builder.

## Next Steps

- Add mesh repair workflows.
- Add self-intersection detection.
- Connect readiness to the future standalone STL adaptive SDF builder.

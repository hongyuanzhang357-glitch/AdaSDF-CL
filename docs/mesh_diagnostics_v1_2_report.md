# AdaSDF-CL v1.2.0-alpha Mesh Diagnostics Report

## Goal

Add public STL mesh diagnostics as a preflight step for future adaptive SDF
builder, FCL fallback, and CollisionWorld work.

This release does not implement a full STL-to-adaptive-SDF builder, FCL backend,
ROS / MoveIt integration, CCD, or mesh repair.

## Added Files

- `include/adasdf/mesh/TriangleMesh.h`
- `include/adasdf/mesh/STLReader.h`
- `include/adasdf/mesh/MeshDiagnostics.h`
- `include/adasdf/mesh/MeshDiagnosticsWriter.h`
- `src/mesh/TriangleMesh.cpp`
- `src/mesh/STLReader.cpp`
- `src/mesh/MeshDiagnostics.cpp`
- `src/mesh/MeshDiagnosticsWriter.cpp`
- `tools/adasdf_mesh_check.cpp`
- `examples/12_mesh_diagnostics_demo.cpp`
- `tests/test_stl_reader.cpp`
- `tests/test_mesh_diagnostics.cpp`
- `tests/test_mesh_diagnostics_writer.cpp`
- `tests/test_mesh_check_cli.cpp`
- `tests/test_mesh_diagnostics_fixtures.cpp`
- `tests/data/mesh_diagnostics/*.stl`
- `docs/mesh_diagnostics.md`
- `docs/stl_import_audit.md`
- `docs/github_release_draft_v1_2_0_alpha.md`

## Modified Files

Version metadata, CMake, README, CHANGELOG, alpha status, roadmap, tools and
examples README, capability docs, validation scripts, public umbrella header,
and reports.

## STL Reader Support

- ASCII STL: supported.
- Binary STL: supported.
- Missing file / empty file: clear failure.
- Binary triangle count: checked against file size.
- Duplicate vertex merge: tolerance-based.

## Diagnostics Items

- vertex and triangle counts;
- AABB and diagonal length;
- NaN / Inf detection;
- degenerate triangles;
- duplicate triangles;
- boundary edges;
- non-manifold edges;
- connected components;
- watertight status;
- isolated vertices;
- scale warnings;
- recommendation text.

## CLI Usage

```bash
adasdf_mesh_check model.stl
adasdf_mesh_check model.stl --out mesh_report.md
adasdf_mesh_check model.stl --json mesh_report.json --tolerance 1e-12
```

Return codes:

- `0`: diagnostics completed without critical issues.
- `1`: read or command-line failure.
- `2`: diagnostics completed with critical mesh issues.

## Fixtures

All fixtures are project-generated ASCII STL files:

- `closed_cube_ascii.stl`
- `open_cube_missing_face_ascii.stl`
- `degenerate_triangle_ascii.stl`
- `duplicate_triangle_ascii.stl`
- `non_manifold_edge_ascii.stl`
- `two_components_ascii.stl`

## Validation Results

- CPU-only Release CTest: 59/59 PASS.
- CUDA Release CTest through an ASCII-only source junction with CUDA v12.6:
  59/59 PASS.
- Install validation: PASS.
- Alpha validation with install validation included: PASS.
- Clean check: PASS.

## Current Limits

- No mesh repair.
- No self-intersection detection.
- No full standalone arbitrary-STL adaptive SDF builder.
- No FCL fallback backend.
- No ROS / MoveIt integration.
- No CCD.

## Next Steps

- Add self-intersection detection.
- Add mesh repair recommendations before repair implementation.
- Define standalone STL-to-adaptive-SDF builder scope.
- Add real robot STL benchmark plans without committing external assets.

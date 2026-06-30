# AdaSDF-CL v1.4.0-alpha Mesh Cleanup Report

## Goal

Add safe, explainable mesh cleanup on top of STL diagnostics and readiness.
The release helps users export a cleaned STL and compare before/after
diagnostics before future SDF construction.

## Added Files

- `include/adasdf/mesh/MeshCleanup.h`
- `src/mesh/MeshCleanup.cpp`
- `include/adasdf/mesh/STLWriter.h`
- `src/mesh/STLWriter.cpp`
- `tools/adasdf_mesh_clean.cpp`
- `examples/13_mesh_cleanup_demo.cpp`
- `tests/test_mesh_cleanup.cpp`
- `tests/test_stl_writer.cpp`
- `tests/test_mesh_clean_cli.cpp`
- `tests/test_mesh_cleanup_roundtrip.cpp`
- `docs/mesh_cleanup.md`
- `docs/github_release_draft_v1_4_0_alpha.md`
- `docs/mesh_cleanup_v1_4_report.md`

## Modified Files

- `tools/adasdf_mesh_check.cpp`
- `include/adasdf/mesh/MeshDiagnosticsWriter.h`
- `src/mesh/MeshDiagnosticsWriter.cpp`
- `include/adasdf/adasdf.h`
- `CMakeLists.txt`
- validation scripts
- README, changelog, status, roadmap, and capability docs

## Cleanup Supported

- Merge near-duplicate vertices.
- Remove degenerate triangles.
- Remove duplicate triangles.
- Remove unused vertices and remap triangle indices.
- Export cleaned ASCII STL.
- Generate before/after diagnostics/readiness reports.

## Cleanup Not Supported

- Hole filling.
- Self-intersection repair.
- Boolean reconstruction.
- Unit conversion.
- Industrial CAD repair.

## STL Writer Status

The v1.4 writer supports ASCII STL export and computes facet normals from
triangle vertex order. Binary STL writing remains planned.

## CLI Usage

```bash
adasdf_mesh_clean bad.stl cleaned.stl --report cleanup_report.md
adasdf_mesh_check bad.stl --readiness --clean-out cleaned.stl --clean-report cleanup_report.md
adasdf_mesh_check cleaned.stl --readiness --out cleaned_report.md
```

## Before / After Report Example

Cleanup reports include:

- Before diagnostics.
- Before readiness.
- Cleanup operations and removed counts.
- After diagnostics.
- After readiness.
- Remaining issues.
- Recommendation.

## Validation Results

- CPU-only Release CTest: 65/65 PASS.
- CUDA Release CTest: 65/65 PASS on local CUDA 12.6 / RTX 4060 Laptop GPU.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.

## Current Limits

Safe cleanup does not guarantee a watertight mesh. It removes obvious duplicate
and degenerate elements but does not perform complex topology reconstruction.

## Next Steps

- Add optional hole-filling workflows with explicit risk controls.
- Add self-intersection detection.
- Connect cleaned STL preflight to the future standalone STL adaptive SDF
  builder.

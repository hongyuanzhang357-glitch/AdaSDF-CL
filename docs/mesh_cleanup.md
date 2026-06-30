# Safe Mesh Cleanup

AdaSDF-CL v1.4.0-alpha adds a conservative mesh cleanup pass for STL
preflight. The goal is to remove obvious mechanical problems before future SDF
construction, not to perform industrial CAD repair.

## Supported Cleanup

- Merge near-duplicate vertices with a configurable tolerance.
- Remove degenerate triangles.
- Remove duplicate triangle candidates, including reversed orientation.
- Remove unused vertices and remap triangle indices.
- Export cleaned meshes as ASCII STL.
- Generate before/after diagnostics and readiness reports.

All operations are optional and can be disabled from the cleanup APIs or CLIs.

## Not Supported

- Hole filling.
- Self-intersection repair.
- Boolean reconstruction.
- Unit conversion.
- Full arbitrary-STL adaptive SDF building.
- FCL fallback.
- Industrial mesh certification.

## Safety Boundary

The cleanup pass never modifies the input STL in place. `adasdf_mesh_clean` and
`adasdf_mesh_check --clean-out` write a new cleaned STL path. If the output path
matches the input path, the command fails.

Removing duplicate or degenerate triangles and merging vertices may change mesh
topology. Reports explicitly mark when topology may have changed. Users should
rerun diagnostics/readiness after cleanup and inspect remaining boundary or
non-manifold issues.

## CLI

```bash
adasdf_mesh_clean bad.stl cleaned.stl --report cleanup_report.md
adasdf_mesh_check bad.stl --readiness --clean-out cleaned.stl --clean-report cleanup_report.md
adasdf_mesh_check cleaned.stl --readiness --out cleaned_report.md
```

Options:

- `--merge-tolerance value`: near-duplicate vertex tolerance.
- `--area-eps value`: degenerate triangle area threshold.
- `--no-merge-vertices`: disable vertex merging.
- `--no-remove-degenerate`: keep degenerate triangles.
- `--no-remove-duplicates`: keep duplicate triangle candidates.
- `--no-remove-unused`: keep unused vertices.

## Recommended Workflow

1. Run `adasdf_mesh_check model.stl --readiness`.
2. Run `adasdf_mesh_clean model.stl cleaned.stl --report cleanup_report.md`.
3. Run `adasdf_mesh_check cleaned.stl --readiness`.
4. If boundary, non-manifold, or self-intersection concerns remain, use a
   dedicated mesh repair/CAD tool before SDF construction.

## Current Limits

This is safe cleanup for SDF build preflight, not CAD repair. It does not fill
holes, repair self-intersections, infer units, or guarantee watertightness.

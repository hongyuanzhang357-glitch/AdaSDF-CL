# AdaSDF-CL v1.2.0-alpha

v1.2.0-alpha adds STL mesh diagnostics and import-audit tooling.

## Added

- ASCII / binary STL reader for diagnostics.
- Public `TriangleMesh` data structure.
- `MeshDiagnostics` for degenerate triangles, duplicate triangles, boundary
  edges, non-manifold edges, connected components, isolated vertices, AABB, and
  scale warnings.
- `MeshDiagnosticsWriter` with Markdown and JSON-like output.
- `adasdf_mesh_check` CLI.
- `examples/12_mesh_diagnostics_demo.cpp`.
- Project-generated STL diagnostic fixtures.
- Mesh diagnostics and STL import audit documentation.

## Notes

- This is a preflight diagnostics release.
- It does not implement mesh repair.
- It does not implement self-intersection detection.
- It does not implement a full standalone arbitrary-STL adaptive SDF builder.
- It does not add FCL fallback, ROS / MoveIt, CCD, or CUDA requirements.

## Validation

- CPU-only CTest: 59/59 PASS locally.
- CUDA CTest: 59/59 PASS locally through an ASCII-only source/build path.
- Install validation: PASS.
- Alpha validation: PASS.

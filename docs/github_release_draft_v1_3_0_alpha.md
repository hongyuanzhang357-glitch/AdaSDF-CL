# AdaSDF-CL v1.3.0-alpha

AdaSDF-CL v1.3.0-alpha adds mesh build readiness scoring and repair
suggestions on top of the v1.2 STL diagnostics path.

## Added

- `MeshReadiness` public module.
- Severity-ranked mesh issues: info, warning, and critical.
- 0-100 SDF build readiness score.
- `Ready`, `UsableWithWarnings`, `Poor`, and `NotRecommended` readiness levels.
- Repair and preprocessing suggestions for common STL problems.
- `adasdf_mesh_check --readiness`.
- Combined diagnostics + readiness Markdown and JSON-like reports.
- Readiness tests and CLI tests.
- Alpha and install validation coverage for readiness reports.

## Notes

- This is a readiness and suggestion release, not an automatic mesh repair
  engine.
- Input STL files are never modified.
- Readiness is a preflight heuristic, not industrial certification.
- Mesh repair, self-intersection detection, and a standalone arbitrary-STL
  adaptive SDF builder remain planned work.

## Validation

- CPU CTest: 61/61 PASS.
- CUDA CTest: 61/61 PASS through an ASCII-only source/build path with CUDA
  v12.6.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.

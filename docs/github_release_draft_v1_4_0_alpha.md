# AdaSDF-CL v1.4.0-alpha

AdaSDF-CL v1.4.0-alpha adds safe mesh cleanup and clean ASCII STL export on top
of the v1.3 diagnostics/readiness workflow.

## Added

- `MeshCleanup` module.
- ASCII `STLWriter`.
- `adasdf_mesh_clean` CLI.
- `adasdf_mesh_check --clean-out` and `--clean-report`.
- Before/after cleanup diagnostics and readiness reports.
- Mesh cleanup demo example.
- Cleanup roundtrip tests.

## Notes

- This release does not fill holes.
- This release does not repair self-intersections.
- Cleanup is a preflight utility for future SDF construction, not an industrial
  CAD repair engine.
- Cleanup writes a separate STL file and refuses to overwrite the input path.

## Validation

- CPU CTest: 65/65 PASS.
- CUDA CTest: 65/65 PASS on local CUDA 12.6 / RTX 4060 Laptop GPU.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.

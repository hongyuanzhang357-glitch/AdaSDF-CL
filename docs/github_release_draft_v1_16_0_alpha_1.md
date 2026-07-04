# AdaSDF-CL v1.16.0-alpha.1

This hotfix tag includes the final v1.16 hierarchical sampling documentation
commit. Use v1.16.0-alpha.1 instead of v1.16.0-alpha.

AdaSDF-CL v1.16.0-alpha.1 is a documentation/tag-alignment hotfix for the
v1.16 hierarchical adaptive sampling release. It does not move the original
`v1.16.0-alpha` tag and does not intend algorithmic changes.

## Highlights

- Includes the final v1.16 hierarchical sampling documentation/report commits.
- Keeps the hierarchical adaptive sampling with quality guard functionality
  introduced in v1.16.0-alpha.
- Preserves the original `v1.16.0-alpha` tag for traceability.
- Adds speed-quality benchmark summary documentation for fixture-level smoke
  measurements.

## Boundaries

- Exact sampling remains the default.
- `.sdfbin` formats are unchanged.
- `--no-quality-guard` does not enable unsafe prediction; the policy falls back
  to exact sampling.
- This hotfix does not implement Tucker/HOSVD, GPU-native compressed query,
  native Python bindings, ROS/MoveIt integration, FCL fallback, CCD, or a
  contact solver.

## Validation

Release validation should include CPU-only build, full CTest, install
validation, alpha validation, Python wrapper tests, repo clean check, and tag
CI on `v1.16.0-alpha.1`.

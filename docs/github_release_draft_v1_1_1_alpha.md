# AdaSDF-CL v1.1.1-alpha

v1.1.1-alpha is a documentation and positioning release. It exposes
AdaSDF-CL's current capability matrix and clarifies implemented, partial,
experimental, demo-only, existing-core-only, and planned features.

## Added

- Capability matrix.
- Implemented / partial / planned feature overview.
- FCL complement strategy documentation.
- Query backend matrix.
- Contact output matrix.
- Public positioning guide.
- `adasdf_capabilities` CLI.
- Capability walkthrough example.
- Capability docs link tests.

## Changed

- README now emphasizes current status, capability boundaries, query modes,
  contact output, accuracy audit, CUDA benchmark semantics, and FCL complement
  strategy.
- Version updated to `1.1.1-alpha`.

## Notes

- No core algorithm changes.
- No public API breaking changes.
- No change to CUDA benchmark semantics.
- No FCL backend, ROS / MoveIt integration, CCD, or mesh repair implementation.
- AdaSDF-CL remains an FCL-style SDF backend under development, not a drop-in
  FCL replacement.

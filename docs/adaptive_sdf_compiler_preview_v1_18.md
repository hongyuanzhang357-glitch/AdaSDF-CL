# AdaSDF-CL Adaptive SDF Compiler Preview

## Scope

AdaSDF-CL v1.18.0-alpha promotes a reviewable STL-to-SDF compiler preview on
top of the existing query and collision runtime. The public path accepts three
hard budgets, emits a versioned `.sdfbin`, reloads it, measures the emitted
asset, and returns either a feasible queryable model or structured infeasible
evidence.

Included compiler capabilities:

- deterministic triangle-BVH exact signed-distance sampling;
- mixed-level adaptive octree refinement and tree statistics;
- deterministic block partitioning and tensor fill provenance;
- Dense, MatrixSVD, HOSVD, TT, and Tucker per-block representations;
- near-zero error/sign guards with dense fallback;
- compressed-direct and decoded-cache runtime query storage;
- bidirectional measured zero-surface validation;
- JSON/CSV reports, CLI creation, and creation benchmark tooling;
- existing distance, collision, sparse query, contact, and world interfaces.

## Public entry points

- `ConstrainedSDFBuilder::fromSTL` and `fromMesh` use the deterministic
  `HeuristicBackendParameterAdvisor`.
- `ConstrainedSDFBuilder::fromSTLWithParameters` and `fromMeshWithParameters`
  use an explicit validated `BackendBuildParameters` value.
- `adasdf_create_sdf` exposes the same contract at the command line.
- `SDFBinReader::read` recognizes `ADASDF_C3D_V1` and returns a
  `ConstrainedSDFModel`.

The heuristic and explicit-parameter paths are validated against the same
build, serialization, reload, accuracy, memory, and file-size checks.

## Alpha boundaries

- `max_zero_surface_abs_error` is enforced against a deterministic,
  bidirectional measured estimate. It is not a mathematical strict bound or a
  certified Hausdorff distance.
- The narrow-band brick path is a compiler preview and can choose dense
  fallback when a low-rank representation is larger or violates its guards.
- GPU-native compressed-direct evaluation is not part of this release. CUDA
  paths continue to use their documented expanded/resident representations.
- The release contains no learned BackendParameterAdvisor implementation,
  training pipeline, calibration layer, surrogate weights, optimizer,
  checkpoint, dataset, or installed model resource.
- No existing query, collision, or legacy `.sdfbin` API is removed.

## Release discipline

Only generic implementation, formal tests, formal tools, and documentation are
staged. Local Horse reports, quality sweeps, advisor V1-V9 experiments,
compiler-optimizer experiments, generated figures, benchmark outputs, and
training artifacts remain outside the release branch.

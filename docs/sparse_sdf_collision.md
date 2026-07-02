# Sparse SDF Collision

AdaSDF-CL v1.9.0-alpha adds sparse point-to-SDF collision as a first-class use
case. It targets small sets of robot link samples, foot contacts, gripper
points, tool probes, sampled geometry, and simulation queries where a full
batch expansion path is unnecessary.

## Core Types

- `CollisionSample` stores `sample_id`, `position`, optional `radius`,
  `object_id`, `link_id`, `group_id`, `weight`, and `label`.
- `CollisionSampleSet` stores a deterministic list of samples and reports a
  radius-aware bounding box.
- `CollisionSampleSetIO` reads and writes CSV sample files.
- `SparseSDFQuery` evaluates samples through `SDFModel::sampleDistance` and,
  only when requested, `SDFModel::sampleGradient`.
- `SparseCollisionQuery` wraps collision-only, clearance, and candidate-search
  modes.

## Sample Radius

Point samples use `radius = 0`. Sphere or proxy samples use:

```text
effective_phi = phi - radius
collision if effective_phi <= threshold
```

This keeps small link/tool proxies cheap while preserving a single scalar
collision predicate.

## Normal Policy

Sparse query defaults to phi-only. It does not compute normals unless the user
requests `--with-normal` or the API sets `compute_normals = true`. Collision-only
early-exit queries therefore avoid unnecessary finite-difference gradient work.

## CLI

```bash
adasdf_sparse_query model.sdfbin samples.csv --threshold 0 --out sparse_results.csv
adasdf_sparse_collide model.sdfbin samples.csv --threshold 0 --early-exit
```

`adasdf_sparse_collide` returns:

- `0` for success with no collision;
- `10` for success with collision detected;
- `1` for read errors;
- `2` for query/report errors.

Return code `10` is a status code, not a program failure.

## Boundaries

Sparse collision is not FCL fallback, not `CollisionWorld`, not CCD, not a full
contact manifold, and not a solver. Direct compressed query is useful for sparse
queries, debugging, fallback, and small point sets. High-throughput GPU query
still belongs to expanded or future active-block paths.

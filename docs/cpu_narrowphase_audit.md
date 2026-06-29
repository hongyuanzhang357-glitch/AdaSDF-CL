# CPU Narrow-Phase Audit

This audit records the v0.3/v0.4+ pair-query fallback state and the v0.5 upgrade into a named `CpuNarrowPhase` research-preview backend.

## Previous Fallback Flow

Before v0.5, `adasdf_cl/src/query/QueryAPI.cpp` owned the whole pair-query fallback:

```text
CollisionObject A/B
  -> world AABB overlap
  -> local candidate points from model AABB and block centers
  -> source transform to world
  -> target inverse transform to target local coordinates
  -> target SDF sample
  -> immediate contact append until max_contacts
```

The code was useful for API validation, but it mixed backend selection, sampling, SDF evaluation, contact creation, distance logic, and statistics in one file.

## Previous Candidate Points

Candidate points came from:

- model local AABB center
- eight local AABB corners
- six local AABB face centers
- a stride-limited subset of block centers from `SDFModel::blockMetadata()`

The candidates were local to the source model and transformed to world coordinates before target SDF query.

## Previous Contact Normal Calculation

The old fallback sampled the target model gradient in target-local coordinates, transformed it with the target rotation, and normalized it. If the gradient was near zero, it used a source-point to target-center fallback.

The normal convention was not explicit enough for symmetric queries, so different raw samples could report opposing normals in the same pair query.

## Previous Minimum Distance Calculation

- If AABBs were separated, `distance()` returned AABB lower-bound distance.
- If AABBs overlapped, it returned the smallest signed distance found among candidate-point SDF samples.
- If no valid sample existed, it fell back to AABB information.

This was approximate and not a global closest-distance solve.

## Previous Contact Reduction

There was no real contact reduction. Contacts were appended while sampling until `max_contacts` was reached. That made the result dependent on candidate order and could discard deeper contacts that were sampled later.

## Previous Transform Handling

- `CollisionObject::getAABB()` transformed all eight local AABB corners, so translations and rotations produce a conservative world AABB for rigid transforms.
- Source candidate points were transformed to world coordinates with `Transform::applyPoint()`.
- Target SDF queries used `Transform::inverseApplyPoint()` to reach target local coordinates.
- Target gradients used `Transform::applyVector()`, so translation was not applied to normals.

This transform path was sound enough to preserve in v0.5.

## Main Error Sources

- Sparse and fixed candidate set.
- No near-surface grid sampling.
- No explicit symmetric-query normal convention.
- No deterministic contact reduction.
- Contact count depended on sample order.
- Overlapping-distance results were approximate SDF samples, not true closest points.
- Direct `sdf::PairCollisionSDF` was not safe to call because native model package, mesh, and transform ownership were not aligned.

## v0.5 Upgrade

v0.5 replaces the monolithic fallback with:

```text
QueryAPI
  -> ExistingPairCollisionBridge if available
  -> CpuNarrowPhase otherwise
      -> world AABB broadphase
      -> CandidatePointSampler
      -> ContactGenerator
      -> ContactReducer
      -> CollisionResult / DistanceResult statistics
```

New behavior:

- `CandidatePointSampler` emits transformed world-space AABB corners, face centers, and near-surface AABB-grid points.
- `ContactGenerator` performs symmetric candidate-point SDF queries.
- Contact normals are stabilized to point from object B toward object A.
- `ContactReducer` sorts raw contacts by penetration depth, merges nearby contacts with similar normals, and enforces `max_contacts`.
- `CollisionResult` and `DistanceResult` now expose candidate, raw contact, reduced contact, backend, method, and SDF-query statistics.

## Remaining Limitations

`CpuNarrowPhase` is still an approximate SDF-sampling research-preview backend. v1.0.0-alpha provides optional CUDA batch query for analytic/demo adaptive boxes, but this narrow-phase does not provide CUDA pair-collision execution, FCL ABI compatibility, a strict global closest-distance solve, or certified industrial contact behavior.

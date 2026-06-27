# Pair Collision Integration Audit

## Scope

This audit checked whether AdaSDF-CL v0.3 can directly route `CollisionObject` pair queries to the parent project's pair collision implementation, especially `sdf::PairCollisionSDF`.

## Relevant Files Found

- `include/sdf/PairCollisionSDF.h`
- `src/PairCollisionSDF.cpp`
- `tests/test_pair_collision_sdf.cpp`
- `include/sdf/SurfaceSampler.h`
- `src/SurfaceSampler.cpp`
- `include/sdf/BVHPairFilter.h`
- `src/BVHPairFilter.cpp`
- `include/sdf/BVHCandidateSampler.h`
- `src/BVHCandidateSampler.cpp`
- `include/sdf/SDFNormal.h`
- `src/SDFNormal.cpp`
- `adasdf_cl/src/io/ExistingSDFBridge.cpp`
- `adasdf_cl/src/query/QueryAPI.cpp`

## Existing Pair Collision Flow

`sdf::PairCollisionSDF::evaluate()` and `evaluateDetailed()` take:

- `sdf::SDFModelPackage` for object A
- `sdf::Mesh` for object A
- `sdf::Transform3D` for object A
- `sdf::SDFModelPackage` for object B
- `sdf::Mesh` for object B
- `sdf::Transform3D` for object B
- `sdf::PairCollisionOptions`

The implementation can use AABB coarse checks, surface sampling, optional BVH candidate filtering, SDF queries, and finite-difference normals. Detailed results contain contact-like point records with world point, local points, normal, signed distance, penetration depth, candidate IDs, and query statistics.

## Reusable Modules

- `sdf::ModelIO::loadBinary()` is already reused by `SDFBinReader::read()`.
- `sdf::AdaptiveLowRankModel::queryPhi()` is already reused through `SDFModel::sampleDistance()`.
- `sdf::PairCollisionSDF` is the right future backend shape for pair collision.
- `sdf::SDFNormal` and candidate sampling code can inform the future narrowphase bridge.

## Not Directly Reusable in v0.3

Direct `PairCollisionSDF` use is blocked because AdaSDF-CL v0.3 exposes a loaded `.sdfbin` as `adasdf::SDFModel` with a private `NativeHandle`. That wrapper supports point distance query, but it does not expose:

- the underlying `sdf::SDFModelPackage`
- the source `sdf::Mesh`
- a reliable loaded mesh associated with the `.sdfbin`
- a native `sdf::Transform3D` conversion path inside a pair bridge

`sdf::PairCollisionSDF` is not a pure two-SDF query over loaded SDF fields; it also needs mesh/surface samples. Loading the mesh from metadata would be fragile because `.sdfbin` assets may be moved independently of their source STL files.

## v0.3 Integration Choice

AdaSDF-CL v0.3 adds `ExistingPairCollisionBridge` as the intended adapter surface, but `ExistingPairCollisionBridge::isAvailable()` returns false. `QueryAPI.cpp` therefore uses a clearly marked CPU fallback:

```cpp
// Fallback CPU query for API validation only.
// TODO: replace with ExistingPairCollisionBridge / adaptive narrowphase.
```

The fallback performs:

1. transform-aware world AABB broadphase
2. candidate sampling from AABB center, corners, face centers, and block centers
3. bidirectional SDF sampling from A into B and B into A
4. finite-difference gradient normals through `SDFModel::sampleGradient()`
5. `CollisionResult`, `DistanceResult`, and `Contact` population

## Limitations

- This fallback is not the final collision algorithm.
- Contact coverage depends on coarse candidate points, not adaptive surface search.
- Minimum distance is approximate when AABBs overlap.
- CUDA batched pair query is not implemented.
- Real FCL integration is not implemented.
- Differentiable Jacobians are not implemented.

## Recommendation

For v0.4 or later pair-query work, extend the existing `.sdfbin` native handle or introduce a controlled internal native package accessor so `ExistingPairCollisionBridge` can obtain `sdf::SDFModelPackage`. Separately, decide whether the bridge should require an explicit mesh path, embed surface samples in a contact-only asset, or derive safe candidate points from SDF block metadata without depending on the original STL path.

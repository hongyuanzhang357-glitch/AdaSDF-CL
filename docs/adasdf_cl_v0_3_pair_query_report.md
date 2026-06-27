# AdaSDF-CL v0.3 Pair Query Report

## 1. Goal

Move AdaSDF-CL from single loaded-SDF point query to a minimal two-object FCL-style query loop:

```text
SDFModel -> CollisionObject + Transform -> distance/collide -> contacts -> tests/examples
```

## 2. Completed

- Added transform-aware `CollisionObject::getAABB()`.
- Added `Transform::fromRotationTranslation()`, `applyPoint()`, `applyVector()`, and `inverseApplyPoint()`.
- Completed `DistanceRequest` and `DistanceResult` runtime state.
- Added `CollisionResult` setters, `numSDFQueries()`, backend info, and timing support.
- Added `ExistingPairCollisionBridge` adapter surface.
- Implemented CPU fallback pair `distance()` and `collide()`.
- Added command-line pair collision and FCL-style examples.
- Added real `.sdfbin` pair-query tests with graceful skip behavior.
- Updated README and design docs for v0.3.

## 3. New Files

- `adasdf_cl/include/adasdf/query/ExistingPairCollisionBridge.h`
- `adasdf_cl/src/query/ExistingPairCollisionBridge.cpp`
- `adasdf_cl/tests/test_collision_object.cpp`
- `adasdf_cl/tests/test_pair_distance_query.cpp`
- `adasdf_cl/tests/test_pair_collision_query.cpp`
- `adasdf_cl/docs/pair_collision_integration_audit.md`
- `adasdf_cl/docs/adasdf_cl_v0_3_pair_query_report.md`

## 4. Modified Files

- `adasdf_cl/CMakeLists.txt`
- `adasdf_cl/README.md`
- `adasdf_cl/docs/api_design.md`
- `adasdf_cl/docs/architecture.md`
- `adasdf_cl/docs/roadmap.md`
- `adasdf_cl/docs/fcl_compatibility.md`
- `adasdf_cl/include/adasdf/adasdf.h`
- `adasdf_cl/include/adasdf/config.h`
- `adasdf_cl/include/adasdf/geometry/Transform.h`
- `adasdf_cl/src/geometry/Transform.cpp`
- `adasdf_cl/include/adasdf/query/CollisionObject.h`
- `adasdf_cl/src/query/CollisionObject.cpp`
- `adasdf_cl/include/adasdf/query/CollisionRequest.h`
- `adasdf_cl/include/adasdf/query/CollisionResult.h`
- `adasdf_cl/src/query/CollisionResult.cpp`
- `adasdf_cl/include/adasdf/query/DistanceRequest.h`
- `adasdf_cl/include/adasdf/query/DistanceResult.h`
- `adasdf_cl/src/query/DistanceResult.cpp`
- `adasdf_cl/src/query/QueryAPI.cpp`
- `adasdf_cl/examples/03_collision_between_two_objects.cpp`
- `adasdf_cl/examples/05_fcl_style_api.cpp`

## 5. PairCollisionSDF Bridge Status

The existing `sdf::PairCollisionSDF` was not directly connected in v0.3.

Reason: it requires `sdf::SDFModelPackage`, `sdf::Mesh`, and `sdf::Transform3D` for both objects. AdaSDF-CL's loaded `SDFModel` currently exposes point query and metadata, but not the native package and mesh needed by the old pair module.

`ExistingPairCollisionBridge` is present as the future integration point and currently reports unavailable.

## 6. Fallback Strategy

The v0.3 backend is a CPU fallback for API validation only:

1. compute transform-aware world AABBs
2. return AABB distance when AABBs do not overlap
3. when AABBs overlap, sample candidate points from AABB center/corners/face centers and block centers
4. query candidates bidirectionally against the other object's `SDFModel::sampleDistance()`
5. compute normals from `SDFModel::sampleGradient()`
6. populate `Contact`, `CollisionResult`, and `DistanceResult`

## 7. CollisionObject Capability

`CollisionObject` now stores an `SDFModel`, transform, object id, and user data. `getAABB()` transforms all eight local AABB corners, so translation and rotation are reflected in the world AABB.

## 8. distance() Implementation

`distance(obj_a, obj_b, request, result)` supports CPU/Auto-style CPU use. For separated AABBs it returns approximate AABB distance and nearest AABB points. For overlapping AABBs it returns the best sampled signed distance and approximate nearest points.

## 9. collide() Implementation

`collide(obj_a, obj_b, request, result)` supports CPU pair query through fallback. It reports collision when any sampled signed distance is below `contact_tolerance`, and optionally collects contacts up to `max_contacts`.

## 10. Contact Fields

`Contact` contains:

- `point`
- `normal`
- `penetration_depth`
- `signed_distance`
- `object_id_a`
- `object_id_b`
- `feature_id_a`
- `feature_id_b`
- `gradient`
- reserved differential data

## 11. Test Results

Command:

```bash
ctest --test-dir build/adasdf_cl-v0_3 -C Debug --output-on-failure
```

Result:

```text
100% tests passed, 0 tests failed out of 9
```

The new tests were:

- `test_collision_object`
- `test_pair_distance_query`
- `test_pair_collision_query`

## 12. Example Results

Commands:

```bash
build/adasdf_cl-v0_3/Debug/adasdf_load_sdfbin_and_query.exe models/horse_tucker_L7_tol025.sdfbin
build/adasdf_cl-v0_3/Debug/adasdf_collision_between_two_objects.exe models/horse_tucker_L7_tol025.sdfbin
build/adasdf_cl-v0_3/Debug/adasdf_fcl_style_api.exe models/horse_tucker_L7_tol025.sdfbin
```

Observed pair-query output summary:

```text
Colliding: true
Minimum distance: -4.30816
Contact count: 8
Number of SDF queries: 1526
```

## 13. Current Limitations

- CPU fallback is approximate and intended only for API validation.
- Direct `sdf::PairCollisionSDF` integration remains TODO.
- High-precision adaptive narrowphase remains TODO.
- CUDA batched pair query remains TODO.
- Real optional FCL adapter remains TODO.
- Python bindings remain TODO.
- Differentiable Jacobians remain TODO.

## 14. Next Recommendation

Proceed to v0.4 Adaptive Builder integration:

```cpp
AdaptiveSDFBuilder::fromMesh("model.stl", build_options)
```

The next pair-query backend step should expose a safe internal native-package/mesh path so `ExistingPairCollisionBridge` can call `sdf::PairCollisionSDF` without relying on UI, viewer, benchmark, or fragile source-path assumptions.

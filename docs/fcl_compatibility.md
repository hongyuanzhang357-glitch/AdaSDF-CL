# FCL Compatibility

AdaSDF-CL is not a replacement for all FCL functionality. It provides a complementary SDF-based collision paradigm for workloads that benefit from signed distance, gradients, contact normals, compression, and parallel query.

## Relationship

FCL and BVH-based libraries are strong general-purpose collision systems. AdaSDF-CL should interoperate with them rather than force a full rewrite:

- FCL/BVH can remain the primary broad geometry system.
- AdaSDF-CL can provide an SDF contact model for selected meshes.
- Users can migrate gradually through familiar object/request/result types.

## Preserved Usage Habits

AdaSDF-CL keeps the following familiar concepts:

- `CollisionObject`
- `CollisionRequest`
- `CollisionResult`
- `DistanceRequest`
- `DistanceResult`

The names are generic and live in the `adasdf` namespace; the implementation should not copy FCL source code or depend on FCL unless an optional adapter target is explicitly enabled.

## Integration Modes

1. Standalone AdaSDF-CL
   - Load `.sdfbin`.
   - Create `CollisionObject`s.
   - Call `collide()` or `distance()`.

2. FCL-style API
   - Use familiar request/result objects.
   - Replace selected query calls with AdaSDF-CL SDF queries.

3. FCL adapter / wrapper
   - Keep existing application object management.
   - Add a wrapper layer that chooses BVH/FCL or SDF/AdaSDF-CL per object or query.

## Adapter Header

`include/adasdf/adapters/FCLAdapter.h` is intentionally dependency-free. It sketches a migration surface without including real FCL headers. A later implementation may add a separate optional target for real FCL interop.

In v0.6, `FCLAdapter::collide()` and `FCLAdapter::distance()` forward to the AdaSDF-CL object-level API and therefore use `CpuNarrowPhase` when no native pair bridge is available. This is a working FCL-style usage surface, not a full FCL integration and not a replacement for FCL broadphase managers, collision geometries, or request/result ABI compatibility.

## Existing Project Notes

The parent project already has `include/sdf/FCLAdapter.h` guarded by `SDF_HAS_FCL`. That code should inform the future optional adapter, but the public AdaSDF-CL interface should keep FCL optional.

The parent project also has `sdf::PairCollisionSDF`, which is closer to a future native pair-query backend than the current research-preview narrow-phase. It currently requires native `sdf::SDFModelPackage`, `sdf::Mesh`, and `sdf::Transform3D` inputs. AdaSDF-CL does not yet expose enough native state and mesh ownership from a loaded `.sdfbin` to call it safely, so `ExistingPairCollisionBridge::isAvailable()` remains false.

## Current Pair-Query Caveat

`CpuNarrowPhase` performs AABB broadphase, symmetric SDF candidate sampling, normal stabilization, and deterministic contact reduction. It improves the quality and observability of the old fallback, but it remains an approximate SDF-sampling narrow-phase. FCL ABI compatibility, FCL broadphase integration, and certified industrial contact behavior are not provided in 0.6.0-alpha.

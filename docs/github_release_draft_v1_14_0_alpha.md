# AdaSDF-CL v1.14.0-alpha

AdaSDF-CL v1.14.0-alpha adds the first CollisionWorld broadphase and
multi-object sample-based SDF collision workflow.

## Highlights

- Added `WorldTransform`, `CollisionGroupMask`, `WorldObject`, and
  `CollisionWorld`.
- Added deterministic `AABBBroadphase` with enabled-state, static-static,
  group/mask, and AABB overlap filtering.
- Added simple CSV scene loading through `WorldSceneIO`.
- Added `WorldSparseCollision` for multi-object sparse SDF collision.
- Added `WorldSolverContacts` for solver-ready contact candidate export across
  world pairs.
- Added CSV, Markdown, and JSON-like world reports.
- Added CLIs:
  - `adasdf_world_broadphase`
  - `adasdf_world_sparse_collide`
  - `adasdf_world_solver_contacts`
  - `adasdf_benchmark_collision_world`
- Added Python wrapper helpers:
  - `world_broadphase`
  - `world_sparse_collide`
  - `world_solver_contacts`
  - `benchmark_collision_world`

## Scope Boundary

CollisionWorld in this release is an SDF-world orchestration layer. It is
sample-based and SDF-native.

It does not implement exact mesh-vs-mesh contact, FCL fallback, a contact
solver, friction, impulses, Jacobian rows, CCD, ROS/MoveIt integration,
pybind11/native Python bindings, or a general scene graph.

`adasdf_world_sparse_collide` returns code `10` when collision is detected.
That return code means successful collision detection, not process failure.

## Validation

- CPU-only CMake build/test/install validation.
- CLI tests for world broadphase, world sparse collision, world solver contact
  export, and CollisionWorld benchmark.
- Python wrapper unittest coverage for the new world helpers.
- Alpha/install validation exercises the installed world CLIs with generated
  `.sdfbin` and sample fixtures outside the source tree.

## Documentation

- `docs/collision_world_scene_format.md`
- `docs/collision_world_v1_14_report.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/alpha_status.md`
- `docs/public_positioning.md`

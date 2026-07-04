# AdaSDF-CL v1.14.0-alpha CollisionWorld Report

## Goal

v1.14.0-alpha adds a first multi-object world orchestration layer on top of the
existing SDF query and sparse contact stack.

The goal is to support deterministic broadphase pair filtering, sample-based
world sparse collision, and solver-ready contact candidate export across
multiple SDF objects without introducing FCL fallback, a physics solver, CCD,
ROS/MoveIt, pybind11, or a scene graph dependency.

## Implemented

- `WorldTransform` for rigid world/object transforms.
- `CollisionGroupMask` for group/mask filtering.
- `WorldObject` for SDF model, samples, transform, type, and enabled state.
- `CollisionWorld` as a deterministic multi-object container.
- `AABBBroadphase` with disabled-object, static-static, group/mask, and AABB
  overlap filtering.
- `WorldSceneIO` for simple CSV scene read/write.
- `WorldSparseCollision` for broadphase pairs plus existing sparse SDF
  narrowphase.
- `WorldSolverContacts` for solver-ready contact candidate export across world
  pairs.
- `WorldReportWriter` for CSV, Markdown, and JSON-like reports.

## CLI Surface

```bash
adasdf_world_broadphase scene.csv --out pairs.csv --report broadphase.md --json broadphase.json

adasdf_world_sparse_collide scene.csv \
  --threshold 0 \
  --early-exit \
  --out world_hits.csv \
  --report world_sparse.md \
  --json world_sparse.json

adasdf_world_solver_contacts scene.csv \
  --threshold 1e-3 \
  --top-k 32 \
  --max-contacts 8 \
  --out world_solver_contacts.csv \
  --report world_solver_contacts.md

adasdf_benchmark_collision_world scene.csv \
  --mode sparse \
  --threshold 1e-3 \
  --repeat 10 \
  --csv collision_world_benchmark.csv
```

`adasdf_world_sparse_collide` returns code `10` for collision detected. This is
the same success-with-collision convention used by the single-model sparse
collision CLI.

## Python Wrapper

The pure-Python CLI wrapper now exposes:

- `world_broadphase`
- `world_sparse_collide`
- `world_solver_contacts`
- `benchmark_collision_world`

The wrapper remains subprocess-based and standard-library only. It is not a
native Python binding.

## Flow

```text
scene.csv
-> WorldSceneIO loads SDF models and optional sample sets
-> CollisionWorld stores enabled objects and transforms
-> AABBBroadphase emits deterministic candidate pairs
-> WorldSparseCollision transforms source samples into target SDF space
-> SparseCollisionQuery evaluates phi/effective_phi
-> WorldSolverContacts reduces world violations into solver-ready candidates
```

## Boundaries

- CollisionWorld uses SDF object AABBs for broadphase.
- Narrowphase is sample-based SDF collision, not exact mesh-vs-mesh contact.
- Solver contacts are candidates for external solvers, not solver constraints.
- No impulses, friction forces, Jacobian rows, LCP/NCP solve, or warm-start
  state are computed.
- No FCL fallback backend is implemented.
- No CCD is implemented.
- No ROS/MoveIt adapter is implemented.
- No pybind11/native Python binding is implemented.

## Validation Coverage

- Unit tests cover transforms, masks, world objects, broadphase filtering,
  scene CSV IO, sparse world collision, solver-ready contacts, report writing,
  and all new CLIs.
- Python tests cover dry-run command construction and benchmark parsing.
- Install validation and alpha validation discover and execute the new world
  tools against a generated validation scene.

## Next Work

- Faster broadphase structures beyond deterministic O(N^2).
- Better scene metadata for robot links and named collision groups.
- Persistent per-object sample caches.
- Optional FCL fallback only after the SDF-only path remains stable.
- Temporal contact matching and warm-start metadata for external solvers.

# FCL Complement Strategy

AdaSDF-CL is not currently a drop-in ABI-compatible replacement for FCL.

## Current Position

AdaSDF-CL is an FCL-style SDF collision backend under development. It
complements FCL by providing signed-distance queries, penetration depth,
contact normals, batch query, expanded-SDF quality audit, and CUDA expanded
query paths. It also includes STL diagnostics, readiness scoring, and safe mesh
cleanup as preflight utilities for future SDF construction.

v1.9 adds sparse point-to-SDF collision and contact candidate extraction. This
is useful for robot link samples, foot contacts, grippers, tools, sampled
geometry, and hard-contact simulations that need bounded candidate counts.

v1.10 adds CPU active block expansion/cache for compressed SDF contact regions.
It reduces runtime memory pressure for repeated local SDF queries, but it is
still not an FCL fallback backend or full broadphase layer.

## Why Not Replace FCL Immediately

FCL has mature mesh collision, broadphase integration, distance queries, and
robotics ecosystem adoption. AdaSDF-CL is still a research-preview SDF backend
with partial contact manifold behavior and optional CUDA query paths.

## What FCL Is Strong At

- Mesh and primitive collision.
- Broadphase collision managers.
- Distance queries over common geometry types.
- Robotics ecosystem integration.
- Mature CCD support in FCL-supported contexts.

## What AdaSDF-CL Is Strong At

- Signed-distance and gradient/normal queries.
- Penetration depth from SDF samples.
- Batch query and CUDA expanded-SDF timing.
- Expanded-SDF quality audit and sign-risk metrics.
- Adaptive SDF research workflow.
- STL diagnostics, readiness scoring, and safe cleanup.
- Sparse point collision with collision-only early exit.
- Sample-radius proxy collision through `effective_phi = phi - radius`.
- Deterministic Top-K contact candidate reduction for contact budgets.

## Task Comparison

| Task | FCL | AdaSDF-CL current | Future hybrid |
| --- | --- | --- | --- |
| mesh collision | strong | existing-core/demo only | FCL mesh frontend + SDF narrowphase |
| broadphase | strong | planned | FCL broadphase or custom CollisionWorld |
| distance query | strong | implemented FCL-style API | selectable backend |
| signed distance | limited | implemented | SDF backend |
| penetration depth | contact dependent | implemented from SDF | SDF refinement |
| contact normal | implemented | implemented research-preview | hybrid validation |
| batch query | limited | implemented CPU/CUDA | SDF acceleration |
| sparse point collision | limited | implemented CPU direct | robot link/sample narrowphase |
| contact candidate budget | contact dependent | implemented Top-K sparse candidates | solver-aware manifold |
| GPU query | not primary | expanded CUDA implemented | hybrid GPU narrowphase |
| error audit | limited | implemented | audit SDF approximations |
| CCD | supported by FCL contexts | planned | FCL CCD fallback |
| robot integration | strong | planned | ROS/MoveIt wrappers |

## Hybrid FCL + SDF Backend Roadmap

1. Use FCL for mesh loading, broadphase, and fallback collision.
2. Use AdaSDF-CL for signed-distance and penetration queries.
3. Add backend selection at CLI/API boundaries.
4. Add quality thresholds to decide whether SDF or FCL fallback is preferred.

## Planned Fallback Modes

```bash
adasdf_collide a.stl b.stl --backend fcl
adasdf_collide a.sdfbin b.sdfbin --backend sdf
adasdf_collide a.stl b.stl --backend hybrid
```

FCL backend is planned, not implemented in v1.10.0-alpha.

Sparse collision in v1.9 does not add FCL fallback, broadphase, CCD,
`CollisionWorld`, or full solver contact constraints.

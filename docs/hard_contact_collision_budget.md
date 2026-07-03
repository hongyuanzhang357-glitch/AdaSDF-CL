# Hard Contact Collision Budget

For hard-contact dynamics, query time is not the only cost. The number and
stability of contact constraints can dominate solver time, especially when a
surface sample set produces many clustered penetrating samples.

AdaSDF-CL v1.9.0-alpha therefore treats sparse collision and contact candidate
generation as separate stages:

- collision-only query answers a boolean question and can stop early;
- clearance query finds the minimum effective phi over the sample set;
- candidate-search query returns threshold violations;
- contact candidate reduction keeps a small, deterministic Top-K set.
- solver-aware stabilization clusters candidates into patches and applies a
  final solver contact budget before export.

## Practical Guidance

- Use collision-only early-exit for fast rejection.
- Use clearance when the minimum distance matters.
- Use candidates when a solver or controller needs a bounded set of likely
  contact points.
- Start with small `top_k` values such as 4 or 8.
- Use `reduction_radius` to avoid clusters of nearly identical constraints.
- For repeated compressed SDF contact queries near the same region, use the
  v1.10 CPU active block cache to expand and reuse only local blocks.

## What v1.9 Does Not Do

v1.9 does not implement stable robot-grade manifold clustering, solver
constraints, FCL broadphase, CCD, or a `CollisionWorld`. Those remain future
integration layers.

v1.10 adds CPU active block expansion/cache for runtime memory control. It is
not CUDA active block residency and not a contact solver. Solver-ready
candidate export is available in v1.13, but impulses, friction forces,
Jacobians, and solver constraints remain out of scope.

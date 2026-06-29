# Limitations

AdaSDF-CL 0.8.0-alpha is a research-preview library layer, not a certified industrial collision stack.

## Backend Availability

- Core-free builds can generate, read, query, and collide demo `.sdfbin` files for analytic boxes.
- The demo `.sdfbin` format is `ADASDF_DEMO_SDFBIN_V1`; it is not the full adaptive compressed SDF format.
- The core-free demo backend currently supports axis-aligned boxes only.
- Full adaptive STL-to-sdfbin generation, adaptive compression, and existing-core `.sdfbin` assets still require existing-core enhanced builds.
- Existing-core downstream consumers may need additional dependency prefixes, such as vcpkg-installed dependencies, in `CMAKE_PREFIX_PATH`.
- The public core-free package is suitable for API review, CMake integration testing, CLI demonstrations, and learning the collision API.

## Pair Collision

- Pair collision uses `CpuNarrowPhase`, an approximate SDF-sampling narrow-phase.
- Candidate points come from transformed AABB corners, face centers, and near-surface AABB-grid samples.
- Contact candidates depend on sampling density and the loaded SDF approximation.
- Contact reduction is a deterministic heuristic that prefers deeper contacts, merges nearby points with similar normals, and caps output by `CollisionRequest::max_contacts`.
- Contact normals are stabilized to point from object B toward object A for a pair query.
- This is a research preview and not a certified contact solver.

## Distance

- `distance()` returns an AABB lower-bound distance when world AABBs are separated.
- When AABBs overlap, `distance()` returns the best signed value found by symmetric candidate-point SDF sampling.
- The result is an approximate SDF-sampling distance, not a strict global closest distance.
- Nearest points are approximate and derived from the sampled point, signed distance, and normal.

## Backends and Adapters

- CUDA batched narrow-phase is not implemented.
- FCL ABI compatibility is not provided.
- `ExistingPairCollisionBridge` remains unavailable until native handles, mesh ownership, and transforms can be aligned safely.
- Python bindings are not implemented.
- Contact-only `.sdfbin` writer/reader remains a design target.
- DASH-SDF surrogate recommendation remains a placeholder unless a future optional backend is added.

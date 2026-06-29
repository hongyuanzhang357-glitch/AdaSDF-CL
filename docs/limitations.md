# Limitations

AdaSDF-CL 0.7.0-alpha.2 is a research-preview library layer, not a certified industrial collision stack.

## Backend Availability

- Core-free builds currently cannot generate, read, or query real `.sdfbin` models.
- Real `.sdfbin` generation, reading, point query, and pair collision currently require existing-core enhanced builds.
- Existing-core downstream consumers may need additional dependency prefixes, such as vcpkg-installed dependencies, in `CMAKE_PREFIX_PATH`.
- The public core-free package is suitable for API review, CMake integration testing, and compile/link smoke tests, but it is not yet a clone-only end-to-end public collision demo.

## Pair Collision

- Pair collision uses `CpuNarrowPhase`, an approximate SDF-sampling narrow-phase.
- Candidate points come from transformed AABB corners, face centers, and near-surface AABB-grid samples.
- Contact candidates depend on the sampling density and the loaded SDF approximation.
- Contact reduction is a deterministic heuristic that prefers deeper contacts, merges nearby points with similar normals, and caps output by `CollisionRequest::max_contacts`.
- Contact normals are stabilized to point from object B toward object A for a pair query.
- This is more stable than the v0.3 API-validation fallback, but it is still a research preview.

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

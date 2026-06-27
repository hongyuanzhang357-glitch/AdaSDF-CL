# Limitations

AdaSDF-CL 0.6.0-alpha is a research-preview library layer, not a certified industrial collision stack.

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

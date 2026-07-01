# Adaptive Octree / Block SDF Builder

AdaSDF-CL v1.6.0-alpha introduces the first public core-free adaptive
octree/block SDF builder.

## Scope

Implemented in v1.6.0-alpha:

- `AdaptiveOctree` and deterministic near-surface refinement.
- `AdaptiveBlockPartitioner`, with one dense block per octree leaf.
- `AdaptiveBlockSDFBuilder` for STL / `TriangleMesh` input.
- `AdaptiveBlockSDFModel` with direct distance and finite-difference gradient queries.
- `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1` read/write support.
- `adasdf_build_adaptive_sdf`.

Not implemented in v1.6.0-alpha:

- Low-rank block compression.
- SVD / Tucker / TT compression.
- Surrogate-guided parameter recommendation.
- GPU-native compressed adaptive query.
- BVH-accelerated build sampling.

## Pipeline

```text
STL
-> STLReader
-> MeshDiagnostics / MeshReadiness
-> optional MeshCleanup
-> AdaptiveOctreeBuilder
-> AdaptiveBlockPartitioner
-> AdaptiveBlockSDFBuilder
-> AdaptiveBlockSDFModel
-> ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
-> SDFBinReader / query / collide / benchmark / expansion quality
```

## Refinement

The v1.6 builder uses an alpha-level deterministic heuristic:

- Always refine until `min_octree_level`.
- Refine sign-changing cells when signed mode is enabled.
- Refine cells whose sampled distance is inside a near-surface band.
- Stop at `max_octree_level`.

All final leaves are retained so the root domain remains covered. Near-surface
status is metadata, not a filter.

## Storage

Each adaptive leaf is stored as a dense `N x N x N` phi grid. This makes the
format inspectable and queryable without external core dependencies. It is not
a compressed representation.

Low-rank compression is planned for v1.7.0-alpha.

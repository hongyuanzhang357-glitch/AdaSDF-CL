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
- Optional CPU BVH-accelerated block sampling in v1.12.0-alpha.
- Optional hierarchical adaptive block sampling with quality guard in
  v1.16.0-alpha.

Not implemented in v1.6.0-alpha:

- SVD / Tucker / TT compression.
- Surrogate-guided parameter recommendation.
- GPU-native compressed adaptive query.
- GPU BVH build sampling.

## Pipeline

```text
STL
-> STLReader
-> MeshDiagnostics / MeshReadiness
-> optional MeshCleanup
-> AdaptiveOctreeBuilder
-> AdaptiveBlockPartitioner
-> AdaptiveBlockSDFBuilder
   (--accel brute by default, optional --accel bvh)
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

v1.7.0-alpha adds a separate compressed adaptive block representation using
matrix-SVD factors and dense fallback. See `low_rank_block_compression.md` and
`stl_to_compressed_sdf_workflow.md`.

## Acceleration

v1.12.0-alpha adds:

```bash
adasdf_build_adaptive_sdf model.stl model_adaptive.sdfbin --accel bvh --threads 4
```

The BVH path accelerates CPU sampling inside adaptive dense blocks. The octree
refinement policy and block storage format remain unchanged.

v1.16.0-alpha adds opt-in hierarchical sampling:

```bash
adasdf_build_adaptive_sdf model.stl model_adaptive.sdfbin \
  --accel bvh --sampling hierarchical --quality-guard
```

Near-surface blocks remain exact by default. Transition and far-field blocks may
use coarse prediction only when deterministic quality checks pass; otherwise the
builder falls back to exact sampling. The SDFBin format is unchanged.

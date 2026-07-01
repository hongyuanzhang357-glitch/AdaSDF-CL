# Low-Rank Block Compression

AdaSDF-CL v1.7.0-alpha introduces a core-free low-rank compression path for
adaptive block SDFs. The first public implementation uses matrix-SVD per
adaptive block.

## Pipeline

```text
STL
-> AdaptiveBlockSDFBuilder
-> block-wise dense phi grids
-> BlockLowRankCompressor
-> Matrix-SVD or DenseFallback blocks
-> CompressedAdaptiveBlockSDFModel
-> ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
```

## Compression Model

Each block phi tensor is flattened into a matrix:

```text
rows = nx
cols = ny * nz
col = iy * nz + iz
```

The compressor stores:

- `U` as `rows * rank`;
- `S` as `rank`;
- `Vt` as `rank * cols`.

Grid values are reconstructed on demand with:

```text
phi(ix, iy, iz) = sum_r U[ix, r] * S[r] * Vt[r, iy * nz + iz]
```

## Rank Selection

v1.7 supports:

- fixed-rank compression;
- error-bounded rank search;
- a simple memory-bounded mode.

Error-bounded selection accepts the first rank that satisfies either the max
absolute error target or the RMS plus p95 targets.

## Dense Fallback

If a block cannot satisfy the requested error target, the default behavior is
to keep that block as dense phi values. This preserves query correctness and
makes mixed compressed/dense assets valid.

## Current Boundaries

- Matrix-SVD block compression is implemented.
- Compression quality audits are implemented.
- Direct compressed CPU query is implemented.
- CUDA workflows use expanded SDF paths after sampling the compressed model.
- Tucker/HOSVD compression is not implemented.
- Surrogate-guided build recommendation is implemented in v1.8.0-alpha through
  `adasdf_recommend_build`.
- The v1.8 recommender is deterministic and experimental; it is not a universal
  trained model, not fully trained, and not an optimality guarantee.
- GPU-native compressed query remains planned.

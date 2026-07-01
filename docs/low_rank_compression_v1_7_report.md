# v1.7.0-alpha Low-Rank Compression Report

## Goal

Implement the first public core-free low-rank compression chain for adaptive
block SDFs:

```text
STL -> AdaptiveBlockSDFBuilder -> dense blocks -> BlockLowRankCompressor
-> CompressedAdaptiveBlockSDFModel -> ADASDF_COMPRESSED_BLOCK_SDFBIN_V1
```

## Added Files

- `include/adasdf/compression/SmallMatrixSVD.h`
- `include/adasdf/compression/CompressedSDFBlock.h`
- `include/adasdf/compression/BlockLowRankCompressor.h`
- `include/adasdf/compression/CompressionQuality.h`
- `include/adasdf/compression/CompressionReportWriter.h`
- `include/adasdf/geometry/CompressedAdaptiveBlockSDFModel.h`
- `include/adasdf/io/CompressedBlockSDFBin.h`
- matching `src/` implementations
- `tools/adasdf_compress_adaptive_sdf.cpp`
- `tools/adasdf_build_compressed_sdf.cpp`
- v1.7 compression examples, tests, and docs

## Design

`SmallMatrixSVD` computes a small-matrix truncated SVD without Eigen, LAPACK, or
MKL. It forms `A * A^T`, runs a Jacobi eigen solve, sorts singular values in
descending order, and stores compact `U/S/Vt` factors.

`BlockLowRankCompressor` flattens each adaptive block as `rows=nx` and
`cols=ny*nz`, selects a rank, reconstructs all grid values, records error
metrics, and falls back to dense storage when the target cannot be met.

`CompressedAdaptiveBlockSDFModel` supports direct CPU query by reconstructing
only the grid values needed by trilinear interpolation. Gradients use finite
difference sampling.

## Format

`ADASDF_COMPRESSED_BLOCK_SDFBIN_V1` stores mixed `MatrixSVD` and
`DenseFallback` blocks plus global compression metadata.

## CLIs

- `adasdf_compress_adaptive_sdf`: compresses an adaptive block `.sdfbin`.
- `adasdf_build_compressed_sdf`: builds STL directly to compressed adaptive
  block `.sdfbin`.
- `adasdf_build_adaptive_sdf --enable-low-rank`: compatibility route for
  compressed output.

## Quality

Compression quality reports include max, mean, RMS, and p95 absolute error,
sign mismatch counts, near-surface sign mismatch counts, memory statistics, and
rank usage.

## Comparison

DenseSDF uses one uniform grid. AdaptiveBlockSDF stores dense per-block grids.
CompressedAdaptiveBlockSDF stores matrix-SVD factors where acceptable and dense
fallback blocks where needed.

## Validation

- CPU-only configure/build: PASS.
- CPU-only CTest: 93/93 PASS.
- CUDA configure/build: PASS with CUDA 12.6 through an ASCII `subst` path.
- CUDA CTest: 93/93 PASS.
- Install validation: PASS.
- Alpha validation with install validation: PASS.
- Clean check: PASS.

## Current Limits

- Tucker/HOSVD compression is not implemented.
- Surrogate-guided recommendation remains planned for v1.8.0-alpha.
- GPU-native compressed query remains planned.
- CUDA benchmark workflows use expanded SDF layouts.

## Next Steps

- v1.8: surrogate-guided recommendation for builder/compression parameters.
- v1.9: GPU-native adaptive/compressed query.
- v2.0: FCL hybrid fallback and CollisionWorld broadphase.

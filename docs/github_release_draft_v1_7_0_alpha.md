# AdaSDF-CL v1.7.0-alpha

AdaSDF-CL v1.7.0-alpha introduces low-rank compression for adaptive block SDFs.
Each adaptive dense block can be compressed using a small matrix-SVD
representation with per-block error auditing and optional dense fallback.

## Added

- Small matrix-SVD utility for block compression.
- `CompressedSDFBlock` and `CompressedAdaptiveBlockSDF` data structures.
- `BlockLowRankCompressor` with fixed-rank and error-bounded rank selection.
- `CompressedAdaptiveBlockSDFModel` direct CPU query path.
- `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1` read/write support.
- Compression quality audit and Markdown/JSON-like reports.
- `adasdf_compress_adaptive_sdf`.
- `adasdf_build_compressed_sdf`.
- Compressed SDF query, collision, benchmark, quality, and roundtrip tests.
- Dense vs adaptive vs compressed comparison examples.

## Changed

- `adasdf_build_adaptive_sdf --enable-low-rank` can emit compressed block SDFs.
- Adaptive builder preview now marks matrix-SVD low-rank block compression as
  implemented.
- `adasdf_info`, `adasdf_query`, `adasdf_collide`, expansion quality, and
  benchmark model loading can use compressed block SDF files.

## Boundaries

- Matrix-SVD block compression is implemented.
- Tucker/HOSVD compression is not implemented in v1.7.0-alpha.
- Surrogate-guided recommendation remains planned for v1.8.0-alpha.
- GPU-native compressed query remains planned.
- CUDA workflows use expanded SDF paths.

## Validation

- CPU-only build: PASS.
- CPU-only CTest: 93/93 PASS.
- CUDA build: PASS with CUDA 12.6 through an ASCII `subst` path.
- CUDA CTest: 93/93 PASS.
- Install validation: PASS.
- Alpha validation with install validation: PASS.
- Clean check: PASS.

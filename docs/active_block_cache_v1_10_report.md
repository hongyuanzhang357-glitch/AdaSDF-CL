# Active Block Cache v1.10 Report

## Goal

AdaSDF-CL v1.10.0-alpha implements a CPU runtime path that avoids global
expansion for compressed SDF contact workflows by expanding only active local
blocks and reusing them through a deterministic cache.

This release is intentionally scoped to CPU active block expansion/cache. CUDA
active block cache, GPU block upload, GPU-native compressed query, FCL fallback,
`CollisionWorld`, and full solver-grade contact manifolds remain planned work.

## New Files

- `include/adasdf/runtime/ActiveBlockSelector.h`
- `include/adasdf/runtime/ExpandedBlock.h`
- `include/adasdf/runtime/ExpandedBlockCache.h`
- `include/adasdf/runtime/BlockExpansionManager.h`
- `include/adasdf/runtime/ActiveBlockQuery.h`
- `include/adasdf/runtime/ActiveBlockReportWriter.h`
- `src/runtime/ActiveBlockSelector.cpp`
- `src/runtime/ExpandedBlock.cpp`
- `src/runtime/ExpandedBlockCache.cpp`
- `src/runtime/BlockExpansionManager.cpp`
- `src/runtime/ActiveBlockQuery.cpp`
- `src/runtime/ActiveBlockReportWriter.cpp`
- `tools/adasdf_select_active_blocks.cpp`
- `tools/adasdf_active_block_query.cpp`
- `tools/adasdf_benchmark_block_cache.cpp`

## Runtime Flow

```text
samples.csv
  -> ActiveBlockSelector
  -> active block ids
  -> BlockExpansionManager
  -> ExpandedBlockCache
  -> ActiveBlockQuery
  -> results.csv / report.md
```

## ActiveBlockSelector

`ActiveBlockSelector` selects deterministic sorted block ids from sparse
samples, contact-candidate-like sample regions, or explicit block id lists.
Selection can account for:

- `threshold`;
- `selection_band`;
- sample `radius`, through the same `effective_phi = phi - radius` convention
  used by sparse collision;
- `extra_margin`;
- optional neighbor block inclusion.

Dense/global models do not need active block expansion and are reported as
unsupported for this runtime path. Adaptive and compressed adaptive block SDF
models are supported.

## Expanded Blocks

`ActiveExpandedBlock` stores a dense local block grid plus metadata needed for
sampling. It is deliberately separate from the older query-layer
`ExpandedBlock` type to avoid ABI/name conflicts while preserving a focused
runtime cache representation.

## ExpandedBlockCache

`ExpandedBlockCache` is a deterministic CPU LRU cache. It tracks resident bytes,
hits, misses, evictions, and cache capacity. The cache is intended for repeated
queries around a small contact or sparse sample region, where expanding a local
working set once is cheaper than reconstructing compressed low-rank values for
every query point.

## BlockExpansionManager

`BlockExpansionManager` expands selected blocks from `AdaptiveBlockSDFModel` and
`CompressedAdaptiveBlockSDFModel`.

For adaptive dense blocks, it copies the stored local phi grid. For compressed
MatrixSVD blocks, it reconstructs the full local grid once into an
`ActiveExpandedBlock`. Dense fallback blocks keep their stored phi values.

## ActiveBlockQuery

`ActiveBlockQuery` queries sparse samples against the selected local expanded
blocks. Rows report whether each sample used the active cache or fell back to
the model query. Optional normal computation remains explicit; phi-only is the
default for sparse performance.

When `fallback_to_model_query` is enabled, samples outside selected resident
blocks still return direct model results. When disabled, those samples are
reported as outside the active cache domain.

## CLI

```bash
adasdf_select_active_blocks model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --out active_blocks.csv \
  --report active_blocks.md

adasdf_active_block_query model.sdfbin samples.csv \
  --threshold 0 \
  --selection-band 1e-3 \
  --cache-mb 64 \
  --out active_query.csv \
  --report active_query.md

adasdf_benchmark_block_cache model.sdfbin samples.csv \
  --repeat 100 \
  --warmup 10 \
  --cache-mb 64 \
  --compare-direct \
  --csv block_cache_benchmark.csv
```

`adasdf_active_block_query` may return status code `10` when collision is
detected. That is a successful status result, not a process failure.

## Python Wrapper

The pure-Python CLI wrapper now exposes:

- `select_active_blocks`;
- `active_block_query`;
- `benchmark_block_cache`.

Example:

```python
import adasdf_cli as adasdf

sel = adasdf.select_active_blocks(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=0.0,
    selection_band=1e-3,
)

res = adasdf.active_block_query(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=0.0,
    selection_band=1e-3,
    cache_mb=64,
)

bench = adasdf.benchmark_block_cache(
    "model_compressed.sdfbin",
    "samples.csv",
    repeat=100,
    compare_direct=True,
)
```

## Validation

- CPU CTest: 121/121 PASS.
- Python unittest: 31/31 PASS.
- Install validation: PASS.
- Alpha validation: PASS with install validation.
- Clean check: PASS.
- CUDA validation: optional; CUDA active block cache is not implemented in
  v1.10.

## Current Limitations

- CPU cache only.
- No CUDA active block residency.
- No GPU block upload path.
- No GPU-native compressed query.
- No FCL fallback or `CollisionWorld`.
- No BVH-accelerated build/query block lookup.
- No full solver-grade contact manifold.

## Next Steps

- v1.11: CUDA active block cache.
- v1.12: BVH-accelerated builder and block lookup improvements.
- v1.13: solver-aware contact manifold work.
- v1.14: `CollisionWorld` and broadphase integration.

# Active Block Expansion Cache

AdaSDF-CL v1.10.0-alpha adds a CPU runtime cache for contact-aware local block
expansion.

The cache is designed for compressed adaptive block SDFs. Instead of globally
expanding every low-rank block into dense memory, it expands only selected
local blocks and keeps those dense blocks in an LRU cache.

## What It Does

- Selects active block ids from sparse collision/contact samples.
- Expands adaptive dense blocks or compressed MatrixSVD/DenseFallback blocks.
- Stores expanded local dense blocks in `ExpandedBlockCache`.
- Queries phi and optional finite-difference normals from resident blocks.
- Falls back to the model direct query when a point is outside resident blocks.
- Reports cache hits, misses, resident memory, fallback count, and query time.

## What It Does Not Do

- It is not global expansion.
- It is not CUDA resident active block cache.
- It is not GPU-native compressed SDF query.
- It is not a solver contact manifold.
- It does not change the compressed `.sdfbin` format.

## Public API

- `ActiveBlockSelector`
- `ActiveExpandedBlock`
- `ExpandedBlockCache`
- `BlockExpansionManager`
- `ActiveBlockQuery`
- `ActiveBlockReportWriter`

The runtime block type is named `ActiveExpandedBlock` to avoid colliding with
the existing `adasdf::ExpandedBlock` type used by `ExpandedSDF`.

## CLIs

```bash
adasdf_select_active_blocks model.sdfbin samples.csv --threshold 0 --out active_blocks.csv
adasdf_active_block_query model.sdfbin samples.csv --threshold 0 --out results.csv
adasdf_benchmark_block_cache model.sdfbin samples.csv --repeat 10 --compare-direct --csv bench.csv
```

`adasdf_active_block_query` returns code `10` when collision is detected. This
means the query succeeded and found a hit.

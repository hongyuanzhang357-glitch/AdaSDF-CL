# Active Block Cache

The active block cache expands a selected local working set of adaptive or
compressed SDF blocks and reuses those expanded dense local blocks for sparse
or contact-oriented runtime queries.

v1.10.0-alpha introduced the CPU active block cache. v1.11.0-alpha added an
optional CUDA active block cache baseline over CPU-expanded active dense
blocks. v1.17.0-alpha adds fast block/cache lookup metadata around the CPU
active cache path.

## v1.17 Lookup Additions

- `BlockLookupIndex` for query point to block id lookup.
- `ActiveBlockHashMap` for block_id to cache slot lookup.
- `ActiveBlockSpatialHash` for query point to resident active block lookup.
- `CacheSlotMap` for combined cache lookup.
- Linear reference and fallback controls.
- Lookup statistics in active block query and block cache benchmarks.

The lookup layer does not change block-local interpolation. Once a block is
selected, the existing expanded block query path still uses direct regular-grid
trilinear interpolation over the eight neighboring nodes.

## CLI

```bash
adasdf_active_block_query model.sdfbin samples.csv \
  --lookup hash \
  --cache-lookup spatial-hash \
  --allow-linear-fallback \
  --report-lookup-stats

adasdf_benchmark_block_cache model.sdfbin samples.csv \
  --lookup hash \
  --cache-lookup spatial-hash \
  --report-lookup-stats
```

Use `--lookup linear` or `--cache-lookup linear` to force the reference scan.
Use `--no-linear-fallback` when diagnosing missed buckets.


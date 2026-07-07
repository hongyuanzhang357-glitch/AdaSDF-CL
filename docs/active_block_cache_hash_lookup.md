# Active Block Cache Hash Lookup

Release: `v1.17.0-alpha`

The active block cache introduced in v1.10 expands a selected local working set
of adaptive or compressed SDF blocks and reuses those expanded dense local
blocks during sparse/contact queries. v1.17.0-alpha adds fast lookup metadata
around that cache.

## Runtime Maps

`ActiveBlockHashMap` maps:

```text
block_id -> cache_slot
```

`ActiveBlockSpatialHash` maps:

```text
query point -> resident active block -> cache_slot
```

`CacheSlotMap` combines both maps so callers can resolve either a known block
id or a query point against the resident active block set.

## CLI Options

`adasdf_active_block_query` and `adasdf_benchmark_block_cache` now accept:

```bash
--lookup linear|hash|morton
--cache-lookup linear|hash|spatial-hash
--allow-linear-fallback
--no-linear-fallback
--report-lookup-stats
```

`--lookup linear` preserves the previous reference behavior. Hash and spatial
hash modes keep exact AABB checks inside candidate buckets. When fallback is
enabled, a miss can fall back to the linear reference path and is reported in
the lookup statistics.

## Result Semantics

Fast cache lookup changes how resident active blocks are found. It does not
change how a block is sampled once selected. Phi and normal values are still
computed by the existing active expanded block interpolation path.

The cache lookup path is considered performance-claim safe only when lookup
result mismatches are zero and phi-difference fields are bounded by the
benchmark tolerance.

## GPU Status

The CUDA active block cache receives a CPU-built flat lookup table export for
future GPU lookup work. v1.17.0-alpha does not add a CUDA hash table and does
not make compressed SVD reconstruction GPU-native.


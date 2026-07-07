# AdaSDF-CL v1.17.0-alpha

AdaSDF-CL v1.17.0-alpha adds persistent active block lookup metadata and fast
block/cache lookup paths for CPU active block runtime queries.

## Highlights

- Adds `MortonKey` and `BlockSpatialKey`.
- Adds `BlockLookupIndex` with linear, spatial-hash, and Morton-sorted lookup.
- Adds `ActiveBlockHashMap`, `ActiveBlockSpatialHash`, and `CacheSlotMap`.
- Adds lookup statistics and report helpers.
- Adds `--lookup`, `--cache-lookup`, fallback controls, and lookup statistics
  reporting to active block query and active block cache benchmarks.
- Adds `adasdf_benchmark_block_lookup`.
- Adds Python wrapper support for active lookup options and block lookup
  benchmarking.
- Prepares CPU-built flat lookup metadata for future CUDA active block lookup.

## Boundaries

- `.sdfbin` formats are unchanged.
- Block-internal 8-node trilinear interpolation still uses direct regular-grid
  indexing.
- Hash/spatial lookup is used for block and cache lookup, not interpolation
  node lookup.
- Linear scan remains available as a reference and optional fallback.
- Speedup claims require zero lookup mismatch and bounded phi difference.
- GPU hash lookup and GPU-native compressed SVD reconstruction remain planned
  work.

## Validation

Use this draft only after GitHub Actions are green for both `main` and the
`v1.17.0-alpha` tag.


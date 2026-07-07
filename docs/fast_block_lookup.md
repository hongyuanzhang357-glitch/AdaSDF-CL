# Fast Block Lookup

Release: `v1.17.0-alpha`

AdaSDF-CL v1.17.0-alpha adds deterministic lookup helpers for runtime active
block query paths. The goal is to avoid scanning every adaptive or resident
active block for every query point while keeping the linear scan path available
as a reference and fallback.

## What Changed

- `MortonKey` encodes and decodes 3D integer coordinates into a stable 64-bit
  key.
- `BlockSpatialKey` combines integer block coordinates, level, and Morton code.
- `BlockLookupIndex` maps query points to containing block ids through:
  - `linear`, the reference scan;
  - `hash`, a spatial hash bucket lookup;
  - `morton`, a sorted Morton-key lookup.
- `BlockLookupDiagnostics` and `BlockLookupReportWriter` expose build time,
  bucket shape, query time, fallback count, and miss count.

## Boundary

This release does not hash the eight interpolation nodes inside a block.
Block-internal trilinear interpolation still uses the existing direct
regular-grid path:

```text
local point -> floor cell coordinate -> eight flattened grid indices
```

Hash and spatial lookup are used only before interpolation:

```text
query point -> containing adaptive block
query point -> resident active block
block_id -> cache slot
```

The `.sdfbin` formats are unchanged. Query results are expected to match the
linear reference path. Any speedup claim must be paired with zero lookup
mismatch and bounded phi difference.

## Fallback

`BlockLookupIndexOptions::allow_linear_fallback` controls whether a hash or
Morton miss may fall back to a linear scan. Fallbacks are counted. Disabling
fallback is useful for diagnostics because misses become visible immediately.

## CUDA Boundary

v1.17.0-alpha does not implement a GPU hash table. `CudaActiveBlockCache`
exports CPU-built flat metadata that can support a future GPU binary search or
flat lookup path:

- keys;
- block ids;
- cache slots;
- block bounds.


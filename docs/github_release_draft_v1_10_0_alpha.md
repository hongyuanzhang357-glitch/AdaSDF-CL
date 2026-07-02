# AdaSDF-CL v1.10.0-alpha

AdaSDF-CL v1.10.0-alpha introduces contact-aware active block expansion and a
CPU expanded-block cache.

## Highlights

- Added `ActiveBlockSelector` for sparse contact-aware block id selection.
- Added `ActiveExpandedBlock`, `ExpandedBlockCache`, and
  `BlockExpansionManager`.
- Added `ActiveBlockQuery` with cache/fallback source reporting.
- Added `ActiveBlockReportWriter`.
- Added `adasdf_select_active_blocks`.
- Added `adasdf_active_block_query`.
- Added `adasdf_benchmark_block_cache`.
- Added Python wrappers:
  `select_active_blocks`, `active_block_query`, and `benchmark_block_cache`.
- Validation now exercises the active block CLI path and Python wrapper dry
  runs.

## Boundary

This release implements a CPU active block cache. It does not implement CUDA
active block residency, full GPU-native compressed SDF query, or solver-grade
contact manifold generation.

`adasdf_active_block_query` returns code `10` when collision is detected. This
is a successful query result, not a program failure.

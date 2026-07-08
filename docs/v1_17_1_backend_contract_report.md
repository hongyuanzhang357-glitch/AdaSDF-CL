# v1.17.1 Backend Contract Report

## Goal

Stabilize AdaSDF-CL backend JSON output and add build profiling/progress/timeout
reporting without changing SDF algorithms or SDFBin formats.

## Existing Schema Search

No existing `adasdf.info`, `adasdf.structure`, `adasdf.block_grid`,
`adasdf.compression`, `adasdf.collide`, `adasdf.benchmark`, or
`adasdf.build_profile` v0.2 Studio schema file was found in the repository.

## Implemented

- Added versioned schemas under `schemas/`.
- Added `include/adasdf/contract` and `src/contract`.
- Added `include/adasdf/profile` and `src/profile`.
- Added model export CLIs for structure, block grid, and compression metadata.
- Added `adasdf_info --json --full`.
- Added collision JSON output with `ADASDF_NO_COLLISION` semantics.
- Added benchmark JSON output for sparse query, block lookup, and contact-band
  benchmark CLIs.
- Added build profile/progress/timeout reporting for dense, adaptive,
  adaptive-compression, and compressed SDF build CLIs.
- Kept progress output separate from JSON stdout.

## Boundaries

- No SDFBin format change.
- No new GPU compressed-query path.
- No native Python binding.
- No FCL ABI or solver API change.

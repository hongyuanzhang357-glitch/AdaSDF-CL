# AdaSDF-CL v1.17.1-alpha

AdaSDF-CL v1.17.1-alpha stabilizes the backend JSON contract and adds build
profile/progress/timeout reporting for automation and Studio integration.

## Highlights

- Versioned JSON schemas under `schemas/`.
- Stable common JSON fields: `schema_id`, `schema_version`, `adasdf_version`,
  `tool_name`, `generated_at_utc`, `status`, `status_code`, `success`, and
  `warnings`.
- Stable `ADASDF_*` status-code strings.
- `adasdf_info --json --full`.
- `adasdf_export_structure --json`, `adasdf_export_block_grid --json`, and
  `adasdf_export_compression --json`.
- `adasdf_collide --json`, including `ADASDF_NO_COLLISION` as a successful
  no-collision result.
- Benchmark JSON output for sparse query, block lookup, and contact-band
  benchmark CLIs.
- Build profile, progress JSONL, timeout guard, `--distance-backend`, and
  `--threads auto` flags for dense, adaptive, adaptive-compression, and
  compressed SDF build CLIs.

## Boundaries

- No `.sdfbin` format change.
- No new SDF algorithm.
- No new GPU compressed-query implementation.
- No FCL-style API change.
- No native Python binding.

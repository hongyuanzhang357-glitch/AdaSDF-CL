# Strict JSON Report Schema

AdaSDF-CL v1.15.0-alpha adds a strict, flat JSON report schema for automation,
benchmark provenance, and regression tracking. The schema is intentionally
implemented without third-party JSON dependencies.

## Schema Version

`adasdf.strict_report.v1`

## Required Fields

| Field | Type | Meaning |
| --- | --- | --- |
| `schema_version` | string | Strict schema id. |
| `adasdf_version` | string | AdaSDF-CL version string. |
| `git_commit` | string | Git commit captured at configure time, or `unknown`. |
| `tool_name` | string | CLI/tool that produced the report. |
| `case_id` | string | User-defined reproducibility case id. |
| `input_path` | string | Primary input path. |
| `input_sha256` | string | SHA-256 of the input when it is a regular file. |
| `output_path` | string | Primary output path. |
| `output_sha256` | string | SHA-256 of the output when it is a regular file. |
| `parameters` | object | Command parameters as strings. |
| `status` | string | `ok`, `failed`, `skipped`, or tool-specific status. |
| `success` | boolean | Whether the run succeeded. |
| `failure_reason` | string | Empty on success. |
| `start_time_utc` | string | UTC timestamp at report timer start. |
| `end_time_utc` | string | UTC timestamp at report write. |
| `elapsed_ms` | number | Wall elapsed time in milliseconds. |
| `platform` | string | `windows`, `linux`, `macos`, or `unknown`. |
| `cpu_threads` | integer | Hardware concurrency reported by C++. |
| `cuda_enabled` | boolean | Whether CUDA was enabled at build configuration. |
| `cuda_available` | boolean | Whether AdaSDF-CL built CUDA backend support. |

## Optional Metrics

`triangle_count`, `vertex_count`, `readiness_score`, `build_time_ms`,
`memory_bytes`, `compressed_memory_bytes`, `compression_ratio`,
`max_abs_error`, `mean_abs_error`, `rms_error`, `p95_error`,
`sign_mismatch_count`, `near_surface_sign_mismatch_count`, `query_time_ms`,
`benchmark_ns_per_query`, `contact_count`, `solver_contact_count`, and
`broadphase_pair_count`.

## CLI Support

The schema is produced by `adasdf_write_manifest` and by selected core CLIs via
`--strict-json report.json --case-id case_id`. Reports can be validated with
`adasdf_validate_report report.json` and summarized with
`adasdf_collect_run_summary --inputs reports.txt --out summary.csv`.

v1.16 extends strict report coverage to
`adasdf_benchmark_hierarchical_sampling`, including speed-quality metrics and
quality gate status for hierarchical sampling comparisons.

## Boundary

The strict JSON report is a reproducibility and automation manifest. It is not
a physics solver output, not a mesh certification report, and not a replacement
for detailed Markdown or benchmark CSV artifacts.

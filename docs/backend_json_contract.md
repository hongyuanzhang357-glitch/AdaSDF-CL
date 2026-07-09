# AdaSDF-CL Backend JSON Contract

Release: `v1.17.2-alpha`

AdaSDF-CL backend JSON output is versioned, deterministic, and dependency-free.
It is intended for Studio, automation, and regression tools that need stable
machine-readable output from CLI tools.

## Common Fields

Every contract JSON object includes:

- `schema_id`
- `schema_version`
- `adasdf_version`
- `tool_name`
- `generated_at_utc`
- `status`
- `status_code`
- `success`
- `warnings`

`status` is one of `ok`, `warning`, or `error`. `status_code` is a stable
`ADASDF_*` string. Process exit code remains separate from JSON `status_code`.

## Supported Schemas

- `adasdf.info.v1`
- `adasdf.structure.v1`
- `adasdf.block_grid.v1`
- `adasdf.compression.v1`
- `adasdf.collide.v1`
- `adasdf.benchmark.v1`
- `adasdf.build_profile.v1`
- `adasdf.error.v1`
- `adasdf.progress.v1`

No `.sdfbin` format change is introduced by this contract.

v1.17.2-alpha adds optional cache fields to `adasdf.build_profile.v1`
timings/counters. They document build-time cache behavior and remain optional
within the v1 schema.

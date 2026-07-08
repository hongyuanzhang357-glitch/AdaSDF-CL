# JSON Schema Reference

Release: `v1.17.1-alpha`

Schema files live under `schemas/` and are installed with the project
documentation. They use fixed schema ids and common required fields for all
backend JSON outputs.

| Schema id | File | Producer |
| --- | --- | --- |
| `adasdf.info.v1` | `schemas/adasdf.info.v1.schema.json` | `adasdf_info --json --full` |
| `adasdf.structure.v1` | `schemas/adasdf.structure.v1.schema.json` | `adasdf_export_structure --json` |
| `adasdf.block_grid.v1` | `schemas/adasdf.block_grid.v1.schema.json` | `adasdf_export_block_grid --json` |
| `adasdf.compression.v1` | `schemas/adasdf.compression.v1.schema.json` | `adasdf_export_compression --json` |
| `adasdf.collide.v1` | `schemas/adasdf.collide.v1.schema.json` | `adasdf_collide --json` |
| `adasdf.benchmark.v1` | `schemas/adasdf.benchmark.v1.schema.json` | benchmark CLIs with `--json` |
| `adasdf.build_profile.v1` | `schemas/adasdf.build_profile.v1.schema.json` | build CLIs with `--profile-json` |
| `adasdf.error.v1` | `schemas/adasdf.error.v1.schema.json` | future structured error output |
| `adasdf.progress.v1` | `schemas/adasdf.progress.v1.schema.json` | progress JSONL events |

The schemas are intentionally small and stable. They validate the contract
surface and leave room for additional payload fields without requiring a new
schema for every internal metric.

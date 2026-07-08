# Progress And Timeout

Release: `v1.17.1-alpha`

Progress events are JSONL objects with schema id `adasdf.progress.v1`.

## Event Fields

- `schema_id`
- `schema_version`
- `adasdf_version`
- `tool_name`
- `generated_at_utc`
- `stage`
- `message`
- `current`
- `total`
- `percent`
- `elapsed_ms`

Stages include `load_mesh`, `adaptive_refinement`, `compression`,
`write_model`, `complete`, `timeout`, and `failed`. Additional stages may be
added while preserving the event shape.

Progress is written to `--progress-json` or stderr through `--progress`; it is
not mixed into JSON stdout outputs.

`--max-seconds` writes a partial build profile with `profile_status=timeout`
and `status_code=ADASDF_TIMEOUT`. Model writing uses a temporary file followed
by rename after reload validation, so timeout and write failures do not leave a
partially written output model as the final path.

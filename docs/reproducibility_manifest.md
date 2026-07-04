# Reproducibility Manifest

AdaSDF-CL v1.15.0-alpha records enough context to compare alpha runs across
machines and CI jobs.

## What Is Captured

- AdaSDF-CL version and configure-time git commit.
- Tool name and user-supplied `case_id`.
- Input and output paths.
- SHA-256 hashes for regular input/output files.
- Command parameters.
- Success/status/failure reason.
- UTC start/end time and elapsed milliseconds.
- Platform, CPU thread count, and CUDA build availability.
- Optional build, quality, benchmark, query, and contact metrics.

## Manual Manifest

```bash
adasdf_write_manifest --case-id cube_build --tool adasdf_build_compressed_sdf \
  --input cube.stl --output cube.sdfbin --out cube_manifest.json \
  --param target_error=1e-3
```

## Tool-Generated Manifest

```bash
adasdf_build_compressed_sdf cube.stl cube.sdfbin \
  --strict-json cube_build.strict.json --case-id cube_build

adasdf_validate_report cube_build.strict.json
```

## Run Summary

Create a text file with one report path per line:

```text
cube_build.strict.json
cube_query.strict.json
cube_world.strict.json
```

Then collect a stable CSV summary:

```bash
adasdf_collect_run_summary --inputs reports.txt --out run_summary.csv
```

The CSV column order is fixed by `adasdf.strict_report.v1`.

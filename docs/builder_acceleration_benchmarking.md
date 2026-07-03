# Builder Acceleration Benchmarking

v1.12.0-alpha adds `adasdf_benchmark_builder_acceleration`.

## Usage

```bash
adasdf_benchmark_builder_acceleration model.stl \
  --builder dense \
  --accel bvh \
  --threads 4 \
  --repeat 3 \
  --warmup 1 \
  --benchmark-brute-reference
```

Supported builders:

- `dense`;
- `adaptive`;
- `compressed`.

The benchmark runs in memory and does not write `.sdfbin` output. It prints a
human-readable summary and a compact CSV-like line:

```text
builder,acceleration,threads,sample_count,bvh_build_time_ms,sampling_time_ms,total_build_time_ms,brute_reference_time_ms,speedup_vs_bruteforce
```

## Interpretation

- `bvh_build_time_ms` is setup cost.
- `sampling_time_ms` is the grid/block sample fill cost.
- `total_build_time_ms` includes the full builder timing scope.
- `brute_reference_time_ms` is emitted only when requested.
- `speedup_vs_bruteforce` compares brute-reference sampling to selected
  sampling time and should not be mixed with query-kernel benchmarks.

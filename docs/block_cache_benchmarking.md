# Block Cache Benchmarking

`adasdf_benchmark_block_cache` compares active-block cached query against
direct sparse query.

Example:

```bash
adasdf_benchmark_block_cache model.sdfbin samples.csv \
  --repeat 10 --warmup 1 \
  --threshold 0 --selection-band 1e-3 \
  --mode phi-only \
  --compare-direct \
  --csv block_cache_benchmark.csv
```

## Reported Fields

- `sample_count`
- `active_block_count`
- `expanded_block_count`
- `cache_memory_bytes`
- `cache_hit_rate`
- `fallback_query_count`
- `active_block_avg_ms`
- `active_block_ns_per_sample`
- `direct_avg_ms`
- `direct_ns_per_sample`
- `mode`
- `threshold`
- `selection_band`

The benchmark is CPU-only in v1.10.0-alpha. CUDA active-block residency is
planned separately.

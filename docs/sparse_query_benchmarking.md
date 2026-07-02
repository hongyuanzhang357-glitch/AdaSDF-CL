# Sparse Query Benchmarking

`adasdf_benchmark_sparse_query` measures CPU sparse point-to-SDF query paths. It
is intentionally separate from `adasdf_benchmark_batch_query`, which focuses on
large batch, expansion, and CUDA timing semantics.

## Example

```bash
adasdf_benchmark_sparse_query model.sdfbin samples.csv \
  --repeat 100 \
  --warmup 10 \
  --mode phi-only \
  --csv sparse_benchmark.csv
```

Supported modes:

- `phi-only`
- `phi-normal`
- `collision-only`
- `clearance`
- `candidates`

Reported fields include `sample_count`, `repeat`, `warmup`, `total_ms`,
`avg_ms`, `avg_us`, `avg_ns_per_sample`, `queried_samples_avg`,
`early_exit_rate`, `mode`, `with_normal`, `threshold`, and `top_k`.

## Interpretation

Sparse query is useful for small point sets, robot link probes, debug checks,
fallback paths, and contact candidate discovery. It is not the main
high-throughput GPU benchmark. Direct compressed query avoids global expansion
but can be slower per point because low-rank values may need to be reconstructed
on demand.

The main runtime memory advantage of compressed SDF should come from expanding
only active blocks near contact or query regions rather than globally expanding
the entire model. v1.10 implements this as a CPU active block cache; CUDA active
block residency remains planned.

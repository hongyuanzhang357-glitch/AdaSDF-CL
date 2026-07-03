# Contact Reduction Benchmarking

`adasdf_benchmark_contact_reduction` measures sparse query plus candidate
reduction and stabilization overhead.

```bash
adasdf_benchmark_contact_reduction model.sdfbin samples.csv \
  --threshold 1e-3 \
  --top-k 64 \
  --max-contacts 8 \
  --repeat 100 \
  --warmup 10 \
  --csv contact_reduction_benchmark.csv
```

Reported fields:

- `sample_count`
- `raw_candidate_count`
- `patch_count`
- `solver_contact_count`
- `candidate_reduction_ratio`
- `avg_query_ms`
- `avg_reduction_ms`
- `avg_total_ms`
- `max_contacts`
- `patch_radius`
- `repeat`
- `warmup`

The benchmark is CPU-only and does not require CUDA, FCL, or the existing core.
It benchmarks candidate export overhead, not a hard-contact solver.

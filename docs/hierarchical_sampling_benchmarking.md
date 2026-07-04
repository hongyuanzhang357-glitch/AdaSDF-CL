# Hierarchical Sampling Benchmarking

`adasdf_benchmark_hierarchical_sampling` compares exact adaptive SDF
construction against the v1.16.0-alpha hierarchical sampling path.

## Example

```bash
adasdf_benchmark_hierarchical_sampling model.stl \
  --max-level 4 \
  --block-resolution 8 \
  --coarse-resolution 3 \
  --quality-check-samples 3 \
  --target-error 1e-3 \
  --csv hierarchical_sampling.csv \
  --report hierarchical_sampling.md \
  --json hierarchical_sampling.json \
  --strict-json hierarchical_sampling_strict.json \
  --case-id hierarchical_sampling_v1_16
```

## Metrics

The benchmark reports:

- exact build time;
- hierarchical build time;
- speedup;
- max, mean, RMS, and p95 error;
- sign mismatch counts;
- near-surface sign mismatch counts;
- exact sample counts;
- predicted sample counts;
- fallback block counts;
- quality gate status.

## Interpretation

Speedup is only meaningful when the quality gate passes and sign mismatch
counts remain acceptable. Some small fixtures may not show speedup because the
benchmark intentionally includes validation overhead.

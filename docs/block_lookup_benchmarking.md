# Block Lookup Benchmarking

Release: `v1.17.0-alpha`

`adasdf_benchmark_block_lookup` compares block lookup and active cache lookup
paths against the linear reference. It is CPU-only and does not require CUDA,
FCL, or the existing research core.

## Example

```bash
adasdf_benchmark_block_lookup model.sdfbin samples.csv \
  --lookup linear,hash,morton \
  --cache-lookup linear,hash,spatial-hash \
  --repeat 5 \
  --csv block_lookup.csv \
  --report block_lookup.md \
  --case-id block_lookup_case
```

## CSV Fields

The benchmark reports:

- case and model identity;
- sample, block, and active block counts;
- lookup and cache lookup modes;
- index and cache map build time;
- linear, hash, Morton, and spatial hash query timing;
- `speedup_vs_linear`;
- `lookup_result_mismatch_count`;
- `phi_mismatch_count`;
- `max_abs_phi_diff`, `rms_phi_diff`, and `p95_phi_diff`;
- cache hit and miss counts;
- cache hit rate;
- linear fallback and miss counts;
- `quality_passed`;
- `performance_claim_allowed`.

`performance_claim_allowed` is true only when the measured row is faster than
linear and passes consistency gates. Benchmark output should never compare a
fast lookup row to the reference without also reporting mismatch and phi-diff
fields.

## Reading Results

`speedup_vs_linear > 1` means the selected lookup mode was faster than the
linear reference in that run. It is not a standalone correctness statement.

`lookup_result_mismatch_count == 0` means the selected lookup mode returned the
same block ids as the linear reference for the tested samples.

The current benchmark records phi-difference fields as consistency gates for
future query-coupled variants. v1.17 lookup itself does not alter block-local
interpolation, so lookup-only rows are expected to keep phi mismatch at zero.


# CUDA Active Block Benchmarking

AdaSDF-CL v1.11.0-alpha adds:

```bash
adasdf_benchmark_cuda_block_cache model.sdfbin samples.csv \
  --repeat 20 \
  --warmup 5 \
  --mode phi-only \
  --compare-direct \
  --csv cuda_block_cache_benchmark.csv \
  --report cuda_block_cache_benchmark.md
```

The benchmark reports:

- CUDA total average time;
- CUDA kernel average time;
- CUDA upload average time;
- CUDA download average time;
- CUDA nanoseconds per sample;
- CPU active block average time;
- direct sparse average time.

`--mode phi-only` measures signed-distance query. `--mode phi-normal` measures
distance plus finite-difference normal output.

Return code `20` means CUDA was unavailable and the benchmark was skipped. It
is expected on CPU-only CI.

## Interpretation

The CUDA active block benchmark is not directly comparable with global
expanded-SDF kernel-only timing unless the timing fields are matched. Its total
time includes CPU selection, CPU block expansion, active block upload, sample
upload, kernel execution, download, and CPU postprocessing. Kernel time is the
closest field for CUDA kernel comparisons.

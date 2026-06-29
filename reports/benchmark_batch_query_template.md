# Batch Query Benchmark Report

- AdaSDF-CL version:
- Commit:
- Tag:
- Date:
- OS:
- Compiler:
- CPU:
- GPU:
- CUDA toolkit:
- CMake configuration:

## Command

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --backend cpu,cuda --out benchmark.csv
```

## Results

```text
N points | CPU total ms | CPU ns/query | GPU total ms | GPU ns/query | Speedup | Max phi error | Max normal error
10,000
100,000
1,000,000
```

## CUDA Status

```text
GPU benchmark: SKIPPED because CUDA backend was unavailable on this machine.
```

## Notes

- Results from different machines are not directly comparable.
- Keep raw large logs out of the repository.
- Small `.md` and `.csv` summaries may be committed.

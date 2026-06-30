# AdaSDF-CL v1.0.3-alpha

CUDA full-query benchmark and expanded-SDF query-path optimization.

## Highlights

- Added explicit benchmark output modes:
  - `--output phi`
  - `--output phi,normal`
- Kept `--phi-only` as a legacy alias for `--output phi`.
- Added `--device-only` / `--no-download` CUDA benchmark mode.
- Improved resident CUDA benchmark repeats with reusable output storage.
- Added CSV fields for workspace reuse, allocation count, workspace memory,
  block lookup counters, download status, correctness status, host memory, and
  layout.
- Improved block-expanded neighbor fallback by avoiding a repeated scan of the
  already-tested center block.
- Kept CUDA optional and CPU-only builds fully supported.

## Benchmark Snapshot

| Scenario | Points | Kernel mean ms | Total mean ms |
| --- | ---: | ---: | ---: |
| CUDA global phi kernel | 1,000,000 | 1.1639 | 9.1419 |
| CUDA global phi+normal full | 1,000,000 | 7.0910 | 26.8891 |
| CUDA block 0,1,2 phi kernel | 1,000,000 | 2.3992 | 11.9925 |
| CUDA global phi device-only | 1,000,000 | 0.8510 | 8.1138 |

## Benchmark Interpretation

`kernel_ms` and `total_ms` are different timing scopes. Do not compare phi-only
kernel time with full query total time.

`output=phi` is the closest AdaSDF-CL comparison to original UI kernel-average
timing. `output=phi,normal` includes finite-difference normal work and is the
full query primitive currently used by contact workflows.

`device-only` skips result download and correctness checks. It is a
performance-analysis mode, not a correctness mode.

## Known Boundaries

- Full low-rank compressed SDF GPU-native query remains planned work.
- Full GPU contact generation and reduction remain planned work.
- Current CUDA acceleration targets expanded/demo SDF query primitives.
- Selected block benchmarks are meaningful for local contact-region points, not
  global uniform point clouds.
- This is an alpha / research-preview release.

## Validation

```text
CPU-only configure/build/test: PASS, CTest 46/46
CUDA ASCII-only configure/build/test: PASS, CTest 46/46
Repo clean check: PASS
```

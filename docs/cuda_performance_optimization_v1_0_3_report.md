# AdaSDF-CL v1.0.3-alpha CUDA Performance Optimization Report

Date: 2026-06-30

## Goal

v1.0.3-alpha reduces CUDA full-query overhead and improves block-expanded
benchmark behavior without changing the full low-rank compressed SDF boundary.
The optimized path still targets the core-free demo / expanded SDF backend.

Do not compare phi-only kernel time with full query total time.

## What Changed

- Added explicit benchmark output modes: `--output phi` and
  `--output phi,normal`.
- Kept `--phi-only` as a legacy alias for `--output phi`.
- Added CUDA `--device-only` / `--no-download` benchmark mode.
- Added reusable output-buffer path for resident CUDA benchmark repeats.
- Added workspace reuse, allocation count, workspace memory, block lookup, and
  no-download CSV fields.
- Refined CUDA block neighbor fallback so the already-tested center block is not
  rescanned during finite-difference neighbor lookup.

## Benchmark Semantics

`kernel_ms` is CUDA event time around the expanded SDF kernel. `total_ms` is the
full benchmark query call for one repeat and can include H2D point upload,
synchronization, D2H result download, CPU postprocess, and output handling.

`output=phi` computes signed distance only. It is closest to original UI
kernel-average timing. `output=phi,normal` computes signed distance plus
finite-difference normals and represents the full query primitive currently used
by contact workflows.

`device-only` synchronizes the CUDA kernel but does not download results or run
correctness checks. It estimates a GPU-side pipeline upper bound.

## v1.0.2 vs v1.0.3

| Scenario | v1.0.2 kernel mean ms | v1.0.3 kernel mean ms | v1.0.2 total mean ms | v1.0.3 total mean ms | Change |
| --- | ---: | ---: | ---: | ---: | --- |
| CUDA global phi | 1.1594 | 1.1639 | 13.7921 | 9.1419 | Kernel flat; total -33.7% |
| CUDA global phi+normal | 11.0974 | 7.0910 | 42.5593 | 26.8891 | Kernel -36.1%; total -36.8% |
| CUDA block 0,1,2 phi | 4.1679 | 2.3992 | 15.3624 | 11.9925 | Kernel -42.4%; total -21.9% |
| CUDA global device-only | NA | 0.8510 | NA | 8.1138 | New no-download mode |
| CPU none phi+normal | NA | NA | 60.5139 | 52.9615 | Local rerun faster; CPU path not the optimization target |

## v1.0.3 Local Benchmark Snapshot

| Scenario | Points | Kernel mean ms | Total mean ms | Notes |
| --- | ---: | ---: | ---: | --- |
| CUDA global phi kernel | 1,000,000 | 1.1639 | 9.1419 | `--output phi --kernel-only --reuse-resident` |
| CUDA global phi+normal full | 1,000,000 | 7.0910 | 26.8891 | `--output phi,normal --reuse-resident` |
| CUDA block 0,1,2 phi kernel | 1,000,000 | 2.3992 | 11.9925 | selected local block domain |
| CUDA global phi device-only | 1,000,000 | 0.8510 | 8.1138 | no D2H, no correctness check |
| CPU none phi+normal | 1,000,000 | NA | 52.9615 | `repeat=3` local reference |

## Block Mode Notes

Block mode performance depends on point distribution. Selected blocks are best
for local contact-region query points. They are not intended to accelerate
global uniform point clouds. v1.0.3 avoids rescanning the center block during
neighbor fallback, but a full deterministic block index map remains future work.

## Workspace Notes

With `--reuse-resident`, the expanded SDF, query device buffers, and benchmark
output storage are reused across warmup/repeat iterations. CSV fields report
`workspace_reused`, `allocation_count`, `workspace_capacity`, and
`workspace_device_memory_mb`.

Pinned host memory and SoA point/normal layouts remain planned experiments.
v1.0.3 records `host_memory=paged` and `layout=aos` for the current default
path.

## Boundaries

- Full low-rank compressed SDF GPU-native query remains planned work.
- Full GPU contact generation and reduction remain planned work.
- CUDA remains optional; CPU-only build/test remains supported.
- CUDA-unavailable benchmark and tests must skip gracefully.

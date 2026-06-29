# AdaSDF-CL v1.0.2-alpha

CUDA benchmark semantics and resident query workspace.

## Highlights

- Added formal benchmark timing breakdown fields.
- Added `--warmup`, `--repeat`, `--kernel-only`, `--reuse-resident`, and
  `--phi-only` benchmark modes.
- Added reusable `CudaQueryWorkspace` for repeated CUDA query buffers.
- Added CUDA expanded-SDF phi-only kernel for signed-distance-only timing.
- Kept CUDA optional and CPU-only builds fully supported.
- Preserved v1.0.1 benchmark compatibility aliases: `query_kernel_ms` and
  `query_total_ms`.

## Benchmark Semantics

`total_ms` is the full query call timing. `kernel_ms` is CUDA event kernel time.
Original UI warmed kernel averages should be compared to:

```bash
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --phi-only --kernel-only --reuse-resident --warmup 10 --repeat 50
```

Full query rows include normals, copies, CPU postprocess, and output allocation.

## Local Validation

CPU-only:

```text
Configure: PASS
Build Release: PASS
CTest Release: 41/41 PASS
```

CUDA, ASCII-only source/build path:

```text
Configure: PASS, CUDA backend ON
Build Release: PASS
CTest Release: 41/41 PASS
```

## Local Benchmark Snapshot

| Scenario | Points | Kernel mean ms | Total mean ms |
| --- | ---: | ---: | ---: |
| CUDA global phi-only kernel | 1,000,000 | 1.1594 | 13.7921 |
| CUDA global full total | 1,000,000 | 11.0974 | 42.5593 |
| CUDA block 0,1,2 phi-only kernel | 1,000,000 | 4.1679 | 15.3624 |
| CPU none | 1,000,000 | NA | 60.5139 |

## Known Boundaries

- Full low-rank compressed SDF native GPU query is not implemented.
- CUDA compressed-direct query remains invalid.
- CUDA pair collision is not an industrial GPU collision pipeline.
- Block-mode CUDA lookup is improved minimally but still not equivalent to the
  original UI optimized dense/block backend.

## Tag

```text
v1.0.2-alpha
```

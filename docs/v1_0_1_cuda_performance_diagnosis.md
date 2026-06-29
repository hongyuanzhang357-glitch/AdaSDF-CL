# AdaSDF-CL v1.0.1-alpha CUDA Performance Diagnosis

Date: 2026-06-29

## 1. Goal

This audit explains why AdaSDF-CL v1.0.1-alpha CUDA / CPU query benchmarks are
much slower than the original UI CUDA dense/block-expanded query reports. It
does not change the core algorithm, does not create a tag, and does not push any
changes.

Temporary local instrumentation was used on a diagnostic branch to split setup,
SDF expansion, SDF upload, query buffer allocation, host/device copies, kernel
time, synchronization, CPU postprocess, and total query time. The public API was
not changed.

## 2. Current Benchmark Data

User-provided v1.0.1 snapshot:

| Mode | Points | Total ms | ns/query |
| --- | ---: | ---: | ---: |
| CPU none | 100k | 5.0764 | 50.764 |
| CPU global | 100k | 17.9953 | 179.953 |
| CUDA global | 100k | 5.2922 | 52.922 |
| CUDA global | 1M | 40.7194 | 40.7194 |
| CUDA block 0,1,2 | 1M | 59.8738 | 59.8738 |

Local diagnostic rerun with profiling:

| Mode | Points | Setup ms | Kernel ms | Total ms | ns/query | Expanded MB | GPU MB | Fallback |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CPU none | 100k | 0.0006 | NA | 5.2292 | 52.292 | 0 | 0 | 0 |
| CPU global | 100k | 5.0516 | NA | 18.9030 | 189.03 | 2.0001 | 0 | 0 |
| CUDA global | 100k | 5.7739 | 1.0809 | 4.8322 | 48.322 | 2.0001 | 2.0001 | 0 |
| CUDA global | 1M | 5.8039 | 10.0373 | 42.0235 | 42.0235 | 2.0001 | 2.0001 | 0 |
| CUDA block all | 1M | 5.2518 | 73.4643 | 105.6085 | 105.6085 | 1.5006 | 1.5005 | 0 |
| CUDA block 0,1,2 | 1M | 3.0590 | 31.0481 | 62.5629 | 62.5629 | 0.7503 | 0.7502 | 0 |

The diagnostic run used the same benchmark executable shape as v1.0.1, with
extra local timing prints enabled by an environment variable.

## 3. Original UI CUDA Reference

Original UI CUDA query reference:

| Original UI mode | Points | Reported metric | Time |
| --- | ---: | --- | ---: |
| block-expanded query | 1M | kernel average | about 1.20 ms |
| global dense query | 1M | kernel average | about 1.24 ms |

Important: the original UI number is a kernel average. It should be compared
first with AdaSDF-CL `query_kernel_ms`, not AdaSDF-CL `query_total_ms`.

Even on the kernel-only field, AdaSDF-CL v1.0.1 is slower:

| Mode | AdaSDF-CL 1M kernel ms | Original UI kernel ms | Ratio |
| --- | ---: | ---: | ---: |
| CUDA global | 10.04 | 1.24 | about 8.1x |
| CUDA block 0,1,2 | 31.05 | 1.20 | about 25.9x |
| CUDA block all | 73.46 | 1.20 | about 61.2x |

So this is not only a reporting mismatch. There is also a kernel and memory
layout gap.

## 4. Timing Semantics

| Measured field | Includes | Excludes | Comparable with original UI kernel time? |
| --- | --- | --- | --- |
| `setup_ms` CPU none | `QueryEngine::prepare()` validation / mode resolution | point generation, CPU reference, query loop, error check, CSV | No |
| `setup_ms` CPU global | CPU `SDFExpander` dense grid sampling | point generation, CPU reference, query loop, error check, CSV | No |
| `setup_ms` CUDA global/block | CPU SDF expansion, CUDA SDF resident upload, upload-side `cudaMalloc`, SDF H2D copies, upload synchronize | query point H2D, query kernel, query D2H, CPU reference, error check, CSV | No |
| `query_kernel_ms` CUDA | CUDA event elapsed time around `expandedSdfKernel` | output vector allocation, host point packing, query `cudaMalloc`, H2D points, D2H phi/normals, CPU postprocess, `cudaFree`, CSV | Partly; it is kernel-only, but computes phi and normals |
| `query_total_ms` CPU none/global | CPU query loop over all points; CPU global includes expanded-grid sample and finite-difference gradient | point generation, CPU reference used by benchmark, error check, CSV | No |
| `query_total_ms` CUDA | output vector allocation, host point packing, query `cudaMalloc`, H2D points, event create, kernel wait/sync, D2H phi/normals, CPU normal conversion, event destroy, `cudaFree`, return overhead | setup expansion/upload, CPU reference, error check, CSV | No |
| `ns_per_query` | derived from `query_total_ms` | setup, CPU reference, error check, CSV | No |
| `max_abs_phi_error`, `max_normal_error` | CPU reference comparison after the query | not included in `query_total_ms` | No |

Current benchmark CSV writing is after timing and is not included in either
`setup_ms` or `query_total_ms`.

## 5. Current CUDA Query Flow

For CUDA expanded query, the flow is:

```text
benchmark point generation
CPU direct reference query for error checking
QueryEngine::prepare()
  SDFExpander::expand()
    CPU samples analytic/demo SDF into dense grid(s)
  CudaResidentExpandedSDF::upload()
    pack block metadata and values
    cudaMalloc resident block/value buffers
    H2D block metadata
    H2D SDF values
    cudaDeviceSynchronize
QueryEngine::queryBatch()
  allocate output vectors on CPU
  pack points into DeviceVec3 AoS buffer
  cudaMalloc device query points
  cudaMalloc device phi
  cudaMalloc device normals
  H2D points
  launch expandedSdfKernel
  cudaEventSynchronize stop event
  D2H phi
  D2H normals
  CPU convert DeviceVec3 normals into output gradients/normals
  cudaFree query buffers
benchmark error check
CSV output
```

The SDF values and block metadata are GPU-resident after `prepare()`. Query
points, phi output, and normal output are not resident; they are allocated,
copied, and freed for each `queryBatch()` call.

## 6. Current CPU Query Flow

CPU none:

```text
for each point:
  model.sampleDistance(point)
  model.sampleGradient(point)
  normalize gradient
```

For the demo adaptive box, this dispatches to a tiny analytic box backend.
There is no dense-grid memory traffic and no interpolation.

CPU global/block:

```text
prepare:
  sample model into dense expanded grid(s)
query:
  for each point:
    trilinear sample phi
    finite-difference gradient
      sample phi at +x, -x, +y, -y, +z, -z
    normalize gradient
```

Expanded CPU query performs more function calls and memory loads than CPU none.

## 7. Diagnostic Breakdown

### 1M CUDA Global

| Component | ms |
| --- | ---: |
| setup expand | 4.6060 |
| setup SDF upload | 1.1790 |
| query total reported | 42.0235 |
| inferred output vector allocation / return overhead | about 9.72 |
| pack points | 4.6613 |
| query `cudaMalloc` | 0.1989 |
| H2D points | 3.9001 |
| kernel event time | 10.0373 |
| kernel sync wall | 10.1320 |
| D2H phi | 1.1317 |
| D2H normals | 2.2664 |
| CPU postprocess normals | 4.5220 |
| query `cudaFree` | 0.3472 |
| profiled inner query total excluding output allocation | 32.3060 |

### 1M CUDA Block 0,1,2

| Component | ms |
| --- | ---: |
| setup expand | 1.9660 |
| setup SDF upload | 1.0741 |
| query total reported | 62.5629 |
| inferred output vector allocation / return overhead | about 9.87 |
| pack points | 5.0921 |
| query `cudaMalloc` | 0.2034 |
| H2D points | 3.5281 |
| kernel event time | 31.0481 |
| kernel sync wall | 31.1294 |
| D2H phi | 1.2016 |
| D2H normals | 2.1128 |
| CPU postprocess normals | 4.7976 |
| query `cudaFree` | 0.3853 |
| profiled inner query total excluding output allocation | 52.6945 |

### 1M CUDA Block All

| Component | ms |
| --- | ---: |
| setup expand | 3.5383 |
| setup SDF upload | 1.6845 |
| query total reported | 105.6085 |
| inferred output vector allocation / return overhead | about 9.65 |
| pack points | 4.8010 |
| query `cudaMalloc` | 0.2221 |
| H2D points | 4.3786 |
| kernel event time | 73.4643 |
| kernel sync wall | 73.5635 |
| D2H phi | 0.9309 |
| D2H normals | 2.3984 |
| CPU postprocess normals | 4.6339 |
| query `cudaFree` | 0.5617 |
| profiled inner query total excluding output allocation | 95.9578 |

The first CUDA row in a process pays a large one-time CUDA allocation/context
cost during upload. For example, the first 10k global upload spent about 41 ms
inside upload-side `cudaMalloc`. Subsequent rows in the same process upload in
about 1 to 2 ms. Original UI benchmark reports usually use warmup/repeat and
kernel averages, so first-use setup costs are not comparable.

## 8. Why CPU None Is Fast

CPU none is querying an analytic demo box, not a compressed adaptive SDF. It
does not touch a dense grid and does not perform trilinear interpolation. It is
just direct signed distance, direct/cheap gradient, and normalization in a tight
loop. That is why it stays around 50 ns/query for 100k to 1M points.

This is a strong CPU baseline but not a realistic compressed-SDF CPU baseline.
It makes CUDA look less impressive for small and medium point counts.

## 9. Why CPU Global Is Slower

CPU global first builds a 64x64x64 dense grid, about 2 MB. Query then samples
expanded dense data. For each normal it uses finite differences, so one point
needs one center phi plus six neighbor phi samples. Each phi sample performs
block/domain checks and trilinear interpolation. That is several memory loads
and function calls per point instead of direct analytic box math.

The 100k CPU global row is about 18.9 ms versus 5.2 ms for CPU none, roughly
3.6x slower in the diagnostic run.

## 10. Why CUDA Global Is Not Much Faster

CUDA global has a kernel-only time of about 1.08 ms at 100k and 10.04 ms at 1M,
but the reported total includes host-side work. At 1M, about 32 ms of inner query
time is visible even before output allocation/return overhead:

- point packing: about 4.66 ms;
- H2D points: about 3.90 ms;
- D2H phi and normals: about 3.40 ms;
- CPU normal postprocess: about 4.52 ms;
- kernel: about 10.04 ms;
- CPU output allocation / return overhead: about 9.72 ms inferred.

The kernel is faster than CPU global, but the benchmark returns full phi plus
normal vectors to the CPU every call. For 1M points this means large host-side
allocation and transfer traffic. Compared with CPU none, the CPU baseline is
also unusually cheap because it is analytic.

## 11. Why CUDA Block 0,1,2 Is Slower Than Global

CUDA block 0,1,2 uses three dense blocks and the benchmark generates points in
the selected contiguous block domain. `fallback_count` is 0 and the errors are
finite, so the slowdown is not caused by CPU fallback or invalid/outside points.

The slowdown is mainly inside the kernel:

- global dense has `block_count=1`;
- selected block mode has `block_count=3`;
- block-all mode has `block_count=6` in this benchmark configuration;
- `sampleExpandedSDF()` linearly scans blocks until it finds the containing
  block;
- each point computes the center phi and six finite-difference neighbor samples;
- therefore block lookup is repeated up to seven times per point;
- neighboring samples can cross block boundaries, increasing branch divergence;
- warps contain points from different blocks, so branch paths and memory regions
  diverge;
- block values are split into per-block ranges rather than one globally
  coherent grid.

At 1M points, kernel times scale with this lookup cost:

| CUDA mode | Blocks | Kernel ms |
| --- | ---: | ---: |
| global | 1 | 10.04 |
| block 0,1,2 | 3 | 31.05 |
| block all | 6 | 73.46 |

This is a kernel-layout problem, not a setup/copy problem.

## 12. GPU Residency And Low-Rank Expansion Status

Current GPU residency:

- SDF block metadata and dense SDF values are GPU-resident after `prepare()`.
- Query points are not resident.
- Query output buffers are not resident.
- Query buffers are allocated and freed per `queryBatch()`.
- CPU output vectors are allocated per query.

Current low-rank status:

- v1.0.1 does not expand low-rank compressed SDF factors on GPU.
- `SDFExpander` samples the current model on CPU into dense global or block
  grids.
- The demo benchmark uses analytic/demo adaptive box data, not existing-core
  compressed low-rank blocks.

## 13. Kernel-Only Timing Status

AdaSDF-CL already records CUDA event kernel time as `query_kernel_ms`. That is
the closest available field to original UI kernel average. However, the current
kernel does more work than the original UI phi-query rows appear to report:

- it computes phi;
- it computes normals by six additional finite-difference samples;
- it performs block lookup for each sample;
- it writes normals for D2H transfer.

There is no official warmup/repeat CLI yet. The diagnostic run used the existing
single-shot benchmark plus temporary local component timers.

## 14. Bottleneck Ranking

For CUDA global 1M:

1. CPU output allocation / return overhead, point packing, H2D/D2H, and CPU
   postprocess together are larger than the kernel.
2. Kernel time is still about 8x slower than the original UI kernel reference.
3. Setup expansion/upload is not in `query_total_ms`, but matters for one-shot
   workloads.
4. First CUDA use pays a large context/allocation warmup cost.

For CUDA block 0,1,2 and block all:

1. Kernel block lookup and repeated finite-difference sampling dominate.
2. Host copies and CPU postprocess are still meaningful but secondary.
3. Setup/upload is small after warmup.
4. No CPU fallback was observed.

Overall bottleneck ranking:

| Rank | Bottleneck | Evidence |
| ---: | --- | --- |
| 1 | Block-mode kernel linear block lookup repeated per finite-difference sample | block all 1M kernel 73.46 ms, block 0,1,2 31.05 ms |
| 2 | Global kernel does phi plus finite-difference normals, not phi-only | global 1M kernel 10.04 ms vs original UI about 1.24 ms |
| 3 | Host output allocation / return overhead | inferred about 9.7 ms at 1M |
| 4 | H2D points + D2H phi/normals | about 7.3 ms at 1M global |
| 5 | CPU point packing and normal postprocess | about 9.2 ms at 1M global |
| 6 | Per-query `cudaMalloc`/`cudaFree` | small in warm rows, but still avoidable |
| 7 | SDF expansion/upload setup | about 3 to 6 ms warm setup; first CUDA row much larger |

## 15. Optimization Recommendations

Recommended smallest next diagnostic task:

- Add an official benchmark mode for warmup/repeat and kernel-only reporting,
  without changing algorithms. Suggested flags:
  - `--warmup N`
  - `--repeat N`
  - `--kernel-only`
  - `--reuse-resident`

Recommended smallest performance task:

- Add a resident query workspace that reuses device point/phi/normal buffers and
  CPU output buffers across repeats. The SDF is already resident; the query
  buffers are not.

Recommended kernel tasks:

- Add a phi-only benchmark/kernel path so AdaSDF-CL can compare directly with
  original UI phi query rows.
- Cache or directly compute block index for block-expanded demo grids instead
  of linearly scanning every block for every finite-difference sample.
- Return or reuse the center sample's block index for neighbor samples.
- Consider a block map / micro-block lookup table like the original UI backend.
- Consider SoA layout for points and normals to reduce packing and improve
  coalescing.
- Consider optional float mode for throughput experiments on consumer GPUs,
  while keeping double as the default correctness mode.

Recommended integration tasks:

- Add a full dense/block SDF backend that mirrors original UI semantics more
  closely.
- Add an existing-core expanded asset bridge so benchmark inputs are comparable
  with original dense/block-expanded `.sdfbin` assets.

## 16. Feature Recommendation Matrix

| Candidate | Recommend? | Reason |
| --- | --- | --- |
| resident SDF handle | Already present | SDF values and block metadata stay resident after `prepare()` |
| resident query workspace | Yes | Removes per-call query buffer allocation and enables repeat benchmarks |
| kernel-only benchmark | Yes | Needed for fair comparison with original UI kernel average |
| warmup/repeat | Yes | Removes first-use CUDA context/allocation noise and reports stable averages |
| pinned memory | Yes, after workspace | H2D/D2H copies are visible at 1M |
| SoA layout | Yes | Current AoS point/normal packing and CPU conversion cost several ms |
| float mode | Optional experiment | Consumer GPUs have weak FP64 throughput; current/default should remain double |
| full dense/block SDF backend | Yes | Needed for apples-to-apples original UI comparison |
| existing-core expanded asset bridge | Yes | Needed to benchmark real adaptive assets, not demo analytic boxes |

## 17. Bottom Line

The headline slowdown has two layers:

1. AdaSDF-CL `query_total_ms` is not comparable with original UI kernel average.
   It includes CPU output allocation, point packing, query buffer allocation,
   H2D/D2H copies, synchronization, CPU postprocess, and frees.
2. AdaSDF-CL `query_kernel_ms` is also slower than the original UI kernel
   reference because the v1.0.1 kernel computes finite-difference normals and
   block mode performs repeated linear block lookup. The original UI reference
   is closer to a warmed, phi-query, dense/block-expanded kernel average.

The current CUDA backend is useful as a correct GPU-resident expanded-query
prototype, but it is not yet a performance-equivalent port of the original UI
CUDA query path.

## 18. v1.0.2-alpha Follow-Up

v1.0.2-alpha implements the smallest recommended follow-up from this audit:

- formal benchmark timing fields for setup, expansion, upload, allocation, H2D,
  kernel, sync, D2H, postprocess, free, and total time;
- `--warmup`, `--repeat`, `--kernel-only`, `--reuse-resident`, and `--phi-only`
  benchmark modes;
- reusable `CudaQueryWorkspace` buffers for repeated CUDA queries;
- a phi-only expanded SDF CUDA kernel;
- a minimal block-lookup improvement that directly uses block 0 for global
  single-block dense SDFs and tries the center block before neighbor fallback.

This follow-up improves benchmark semantics and removes repeated query-buffer
allocation in resident benchmark loops. It does not implement full low-rank
compressed SDF native GPU query.

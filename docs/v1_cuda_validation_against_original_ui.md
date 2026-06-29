# AdaSDF-CL v1 CUDA Validation Against Original UI

Date: 2026-06-29

This report validates AdaSDF-CL v1.0.1-alpha query-mode CUDA behavior against
the implementation pattern found in the original OctreeBlockLowRankSDF UI and
benchmark tooling.

## Local Environment

Observed AdaSDF-CL state during validation:

- AdaSDF-CL branch: `main`
- Base published commit before v1.0.1 work: `f4507db Add CUDA batch query benchmark for v1.0.0-alpha`
- Local tag present before v1.0.1 work: `v1.0.0-alpha`
- Working tree during validation: v1.0.1-alpha changes staged for testing
- Remote: `origin` points at the AdaSDF-CL GitHub repository

CUDA and compiler availability:

| Check | Result |
| --- | --- |
| NVIDIA driver/GPU | PASS; local RTX Laptop GPU visible |
| CUDA Toolkit / `nvcc` | PASS; CUDA 12.6 compiler available |
| CMake / Visual Studio generator | PASS; Visual Studio generator compiles CUDA when source/build paths are ASCII-only |

## Non-ASCII Path Finding

The original project root includes non-ASCII characters. CUDA/MSBuild failed in
earlier validation when `.cu` compilation was attempted directly under that
path. An ASCII-only source junction and ASCII-only build directory allowed CUDA
configuration, compilation, and runtime tests to pass.

Conclusion: on this Windows machine, CUDA validation should use an ASCII-only
source/build path before treating CUDA compiler errors as AdaSDF-CL code
issues. This is documented in `docs/gpu_backend.md`.

## AdaSDF-CL v1.0.1 CUDA Build And Tests

CUDA validation configuration:

```text
ADASDF_CL_ENABLE_CUDA=ON
ADASDF_CL_BUILD_TESTS=ON
ADASDF_CL_BUILD_BENCHMARKS=ON
ADASDF_CL_USE_EXISTING_CORE=OFF
```

Configuration result:

- Version: `1.0.1-alpha`
- Existing core requested: OFF
- CUDA backend: ON
- CUDA toolkit: found

Build result:

- PASS
- `src/backend/CudaQueryKernels.cu` compiled with `nvcc`
- `adasdf_cl_runtime` built
- `adasdf_query_mode_demo` built
- `adasdf_benchmark_batch_query` built
- CUDA test executables built

CTest result:

```text
100% tests passed, 0 tests failed out of 37
```

CUDA-relevant runtime tests passed:

- `test_cuda_query_backend`
- `test_cpu_gpu_alignment`
- `test_query_engine_cuda`
- `test_query_mode_cli`

## Query Mode Smoke Tests

Representative CLI results:

| Command mode | Result |
| --- | --- |
| `--backend cpu --expansion none --points 100000` | PASS; prepared backend `cpu`, expansion `none`, status `ok` |
| `--backend cpu --expansion global --points 100000` | PASS; prepared backend `cpu`, expansion `global`, status `ok` |
| `--backend cuda --expansion global --points 100000` | PASS; prepared backend `cuda`, expansion `global`, status `ok` |
| `--backend cuda --expansion none --points 100000` | FAIL as expected with `CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.` |

## AdaSDF-CL v1.0.1 Benchmark Results

Benchmark command family:

```text
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend <cpu|cuda> --expansion <none|global|block> [--blocks all|0,1,2]
```

Summary:

| Backend | Expansion | Blocks | Points | Setup ms | Kernel ms | Total ms | ns/query | Expanded MB | GPU MB | Status |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| CPU | None | all | 100,000 | 0.0008 | NA | 5.0764 | 50.764 | 0 | 0 | ok |
| CPU | Global | all | 100,000 | 4.5588 | NA | 17.9953 | 179.953 | 2.0001 | 0 | ok |
| CPU | Block | all | 100,000 | 3.4537 | NA | 19.8335 | 198.335 | 1.5006 | 0 | ok |
| CUDA | Global | all | 100,000 | 5.8217 | 1.0820 | 5.2922 | 52.922 | 2.0001 | 2.0001 | ok |
| CUDA | Block | all | 100,000 | 5.43 | 7.7973 | 12.1534 | 121.534 | 1.5006 | 1.5005 | ok |
| CUDA | Block | 0,1,2 | 100,000 | 2.7632 | 3.5393 | 7.3181 | 73.181 | 0.7503 | 0.7502 | ok |
| CUDA | Global | all | 1,000,000 | 5.8224 | 10.4934 | 40.7194 | 40.7194 | 2.0001 | 2.0001 | ok |
| CUDA | Block | 0,1,2 | 1,000,000 | 2.6606 | 30.8537 | 59.8738 | 59.8738 | 0.7503 | 0.7502 | ok |

Expanded-grid rows report non-zero phi and normal error against CPU direct
analytic reference. This is expected for finite-resolution dense interpolation
and finite-difference normals.

## Comparison With Original UI CUDA

| Area | Original UI / benchmark | AdaSDF-CL v1.0.1-alpha |
| --- | --- | --- |
| CUDA build switch | `SDF_ENABLE_CUDA=ON` | `ADASDF_CL_ENABLE_CUDA=ON` |
| Missing CUDA behavior | Original project fails configure when `nvcc` is missing | AdaSDF-CL keeps CPU build and reports CUDA backend OFF |
| UI integration | Python UI launches CUDA `sdf_tool` through `QProcess` | No UI; benchmark is a standalone CLI |
| Model type | Existing adaptive `.sdfbin` dense/block-expanded query | Core-free analytic/demo adaptive box expanded through `SDFExpander` |
| GPU storage | Global dense and block-expanded buffers | Global dense and block-dense resident buffers |
| Compressed factor direct query | Explicitly unsupported | Explicitly unsupported; `CUDA + None` is invalid |
| Contact pipeline | Prototype all-points dense-grid contact | CUDA pair collision not implemented |
| Correctness checks | CPU reference phi, diff, sign mismatch | CPU direct reference, phi error, normal error |
| Timing style | Kernel-only plus H2D/D2H/setup/total columns | Setup, CUDA event kernel time, and end-to-end query time |

AdaSDF-CL v1.0.1 therefore matches the original UI's important structural
lesson: GPU query should operate on an explicit dense or block-expanded resident
layout, and unsupported compressed-direct GPU query should be reported
honestly. It does not claim parity with the original project's full adaptive
asset pipeline or all-points contact prototype.

## Limitations

- No full low-rank compressed SDF GPU expansion was tested.
- No direct bridge to original `CudaGlobalDenseSDF` was added.
- No CUDA pair-collision or all-points contact path was added.
- No GPU benchmark was run through the original Python UI in this validation.
- Expanded-grid normals are finite-difference approximations.
- CUDA builds under non-ASCII source/build paths remain unreliable with the
  current local Windows CUDA/MSBuild toolchain.

## Recommendation

The next pre-release should focus on `v1.1.0-alpha` dense-grid quality and
integration work:

- add stronger finite-grid accuracy controls;
- add optional sign-mismatch metrics to the benchmark;
- define the public bridge from existing adaptive assets into `ExpandedSDF`;
- keep compressed-direct CUDA query unsupported until a real low-rank GPU path
  exists.

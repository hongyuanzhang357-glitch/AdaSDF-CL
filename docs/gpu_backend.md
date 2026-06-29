# Optional CUDA Batch Query Backend

AdaSDF-CL v1.0.0-alpha adds an optional CUDA batch query backend.

## Scope

Supported first:

- core-free `AnalyticSDFModel` boxes;
- core-free `DemoAdaptiveSDFModel` boxes;
- batch signed distance;
- batch gradient / normal output;
- CPU/GPU numerical alignment checks.

Not supported yet:

- full low-rank compressed SDF GPU expansion;
- arbitrary mesh/STL GPU SDF construction;
- certified industrial GPU collision pipeline;
- FCL ABI compatibility.

## CMake

CUDA is disabled by default:

```bash
cmake -S . -B build -DADASDF_CL_ENABLE_CUDA=OFF
```

To request CUDA:

```bash
cmake -S . -B build-cuda -DADASDF_CL_ENABLE_CUDA=ON
```

If CUDA is requested but unavailable, configure continues and builds the CPU runtime. The summary reports:

```text
CUDA backend: OFF
CUDA toolkit: not found
```

If CUDA is available:

```text
CUDA backend: ON
CUDA toolkit: found
```

## Public API

```cpp
adasdf::BatchQueryInput input;
input.points = points;

if (adasdf::CudaQueryBackend::isAvailable()) {
  adasdf::BatchQueryOutput output =
      adasdf::CudaQueryBackend::queryAnalyticBox(*model, input);
}
```

CPU-only users can include `<adasdf/adasdf.h>` without including CUDA headers.

## Precision

The v1.0.0-alpha CUDA kernel uses double precision to align with the CPU `Scalar` type. Float mode is intentionally not part of this alpha.

## Testing

CUDA tests are compiled in CPU-only builds and return success with a `SKIPPED` message when CUDA is unavailable. When CUDA is available, the alignment test checks max signed-distance and normal errors against tight tolerances on 1000 deterministic points.

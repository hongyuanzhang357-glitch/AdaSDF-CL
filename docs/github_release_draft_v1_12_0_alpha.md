# AdaSDF-CL v1.12.0-alpha

AdaSDF-CL v1.12.0-alpha adds CPU TriangleBVH acceleration for SDF builder
sampling and deterministic stdlib multi-threaded sampling.

## Highlights

- Optional `--accel bvh` for DenseSDF, AdaptiveBlockSDF, and
  CompressedAdaptiveBlockSDF builders.
- `--accel brute` remains the default and reference path.
- `--threads N` enables deterministic parallel sampling.
- `--benchmark-brute-reference` reports brute sampling time and speedup.
- New `adasdf_benchmark_builder_acceleration` tool.
- Pure-Python CLI wrapper parameters for builder acceleration and benchmark
  parsing.

## Boundaries

- No GPU BVH.
- No FCL fallback.
- No CollisionWorld broadphase.
- No solver-aware full contact manifold.
- No pybind11/native Python binding.
- No low-rank math change.
- Signed builds still depend on watertight input meshes.

## Validation

- CPU Release build: PASS.
- CTest: 141/141 PASS.
- CUDA remains optional and is not required for this release.

## Tag

```text
v1.12.0-alpha
```

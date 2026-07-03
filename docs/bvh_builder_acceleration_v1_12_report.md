# AdaSDF-CL v1.12.0-alpha BVH Builder Acceleration Report

## Goal

Add a CPU-only TriangleBVH acceleration option for STL-to-SDF builder sampling
without changing the public query algorithms, compression math, CUDA paths, or
default brute-force behavior.

## Implemented

- `TriangleAABB`, `TriangleBVH`, `TriangleBVHBuilder`.
- `BVHNearestTriangleQuery` for nearest-triangle pruning.
- `BVHRayIntersectionQuery` for deterministic ray-count sign estimation.
- `BVHSDFSampler` for brute or BVH SDF sampling.
- `ParallelSampling` for deterministic stdlib multi-threaded sampling.
- Builder options: acceleration, threads, and brute-reference benchmarking.
- CLI options: `--accel`, `--threads`, and `--benchmark-brute-reference`.
- Builder acceleration benchmark CLI.
- Python wrapper parameters and parsers.

## Builder Paths

DenseSDF samples each uniform grid point through either brute-force triangle
scan or BVH nearest-triangle query. AdaptiveBlockSDF samples each dense value
inside each adaptive leaf block through the same sampler. The compressed
builder inherits the adaptive sampling path before Matrix-SVD compression.

## Validation Snapshot

- CPU configure: PASS.
- CPU Release build: PASS.
- CTest: 141/141 PASS.
- Python wrapper unittest: 34/34 PASS.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.

## Non-Goals

- GPU BVH.
- FCL fallback.
- Low-rank SDF math changes.
- CollisionWorld integration.
- Contact solver manifold generation.
- Native Python extension bindings.

## Next Steps

- Benchmark larger meshes to quantify builder speedup.
- Evaluate SAH or binned split heuristics behind the existing method enum.
- Add more robust sign classification for difficult closed meshes.
- Consider pinned or memory-pooled host buffers only if builder profiling shows
  allocation overhead matters.

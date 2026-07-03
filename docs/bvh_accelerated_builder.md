# BVH Accelerated Builder

AdaSDF-CL v1.12.0-alpha adds an optional CPU TriangleBVH path for STL-to-SDF
builder sampling.

## Scope

Implemented:

- `--accel brute` keeps the original brute-force reference path.
- `--accel bvh` builds a deterministic median-split TriangleBVH.
- DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF builder CLIs can
  use the BVH sampler.
- `--threads N` enables deterministic stdlib-based parallel sampling.
- `--benchmark-brute-reference` reports brute-reference time and speedup.

Not implemented:

- GPU BVH.
- FCL fallback.
- CollisionWorld broadphase.
- Solver-aware contact manifolds.
- Native Python bindings.
- Changes to Matrix-SVD or low-rank math.

## CLI

```bash
adasdf_build_dense_sdf model.stl dense.sdfbin --accel bvh --threads 4
adasdf_build_adaptive_sdf model.stl adaptive.sdfbin --accel bvh --threads 4
adasdf_build_compressed_sdf model.stl compressed.sdfbin --accel bvh --threads 4
```

The default remains `--accel brute` so the historical reference behavior is
preserved unless users explicitly select BVH.

## Reporting

Builder stdout and Markdown/JSON-like reports include:

- acceleration mode;
- whether BVH was used;
- requested and used thread counts;
- BVH build time;
- sampling time;
- node, leaf, and valid triangle counts;
- optional brute-reference timing and speedup.

## Determinism

The BVH builder uses stable sorting by centroid and triangle id. Parallel
sampling uses fixed contiguous chunks. Output values are written back to fixed
indices, so result grids are deterministic for a given input, option set, and
compiler floating-point mode.

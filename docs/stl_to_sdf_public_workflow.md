# Public STL-To-SDF Workflow

AdaSDF-CL v1.5.0-alpha adds a clone-only, core-free path from STL to a queryable
uniform dense SDF. v1.6.0-alpha adds a clone-only, core-free path from STL to
an adaptive octree/block SDF with dense values per block.

```text
STL
-> STLReader
-> MeshDiagnostics / MeshReadiness
-> optional MeshCleanup
-> DenseSDFBuilder
-> DenseSDFModel
-> ADASDF_DENSE_SDFBIN_V1
-> SDFBinReader
-> adasdf_info / adasdf_query / adasdf_collide / benchmark / quality audit
```

## CLI Example

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_build_dense_sdf model_clean.stl model_dense.sdfbin --resolution 64 --padding 0.05 --report dense_report.md
adasdf_info model_dense.sdfbin
adasdf_query model_dense.sdfbin --point 0 0 0
adasdf_collide model_dense.sdfbin model_dense.sdfbin --max-contacts 4
adasdf_expansion_quality model_dense.sdfbin --expansion global --global-resolution 32 --samples 1000 --out quality.csv
adasdf_benchmark_batch_query --model model_dense.sdfbin --points 10000 --query-backend cpu --expansion none --out bench.csv
```

## Adaptive Block CLI Example

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_build_adaptive_sdf model_clean.stl model_adaptive.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --report adaptive_report.md
adasdf_info model_adaptive.sdfbin
adasdf_query model_adaptive.sdfbin --point 0 0 0
adasdf_collide model_adaptive.sdfbin model_adaptive.sdfbin --max-contacts 4
adasdf_expansion_quality model_adaptive.sdfbin --expansion global --global-resolution 32 --samples 1000 --out quality.csv
adasdf_benchmark_batch_query --model model_adaptive.sdfbin --points 10000 --query-backend cpu --expansion none --out bench.csv
```

## Boundaries

The implemented public builders are a uniform DenseSDF builder and an adaptive
octree/block dense builder. v1.6 does not implement low-rank compression,
SVD/Tucker compression, surrogate recommendation, or GPU-native compressed
adaptive query.

Open STL meshes can be used in unsigned mode:

```bash
adasdf_build_dense_sdf open_mesh.stl open_mesh_dense.sdfbin --resolution 64 --unsigned
```

Signed mode requires watertight input by default:

```bash
adasdf_build_dense_sdf closed_mesh.stl closed_mesh_dense.sdfbin --resolution 64 --signed
```

Generated `.sdfbin`, `.csv`, `.svg`, `.log`, build, and install artifacts
should stay outside the source tree.

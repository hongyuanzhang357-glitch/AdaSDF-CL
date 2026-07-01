# Public STL-To-SDF Workflow

AdaSDF-CL v1.5.0-alpha adds a clone-only, core-free path from STL to a queryable
uniform dense SDF.

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

## Boundaries

The implemented public builder is a uniform DenseSDF builder. It is not the
full adaptive octree/block/low-rank compressed builder. The future adaptive
builder remains planned work.

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

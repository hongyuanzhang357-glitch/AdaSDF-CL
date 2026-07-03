# STL To Adaptive Block SDF Workflow

v1.6.0-alpha adds a public core-free STL-to-adaptive-block-SDF path.

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_recommend_build model_clean.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --emit-command
adasdf_build_adaptive_sdf model_clean.stl model_adaptive.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --accel bvh --threads 4 --report adaptive_report.md
adasdf_info model_adaptive.sdfbin
adasdf_query model_adaptive.sdfbin --point 0 0 0
adasdf_collide model_adaptive.sdfbin model_adaptive.sdfbin --max-contacts 4
```

Signed builds require watertight input by default. Open meshes can be built as
unsigned fields:

```bash
adasdf_build_adaptive_sdf open_mesh.stl open_mesh_adaptive.sdfbin --unsigned --max-level 4
```

The builder writes `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`. Blocks store dense phi
values.

For compressed adaptive block output in v1.7.0-alpha, use either:

```bash
adasdf_compress_adaptive_sdf model_adaptive.sdfbin model_compressed.sdfbin --target-error 1e-3 --max-rank 8
adasdf_build_compressed_sdf model_clean.stl model_compressed.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --max-rank 8
```

See `stl_to_compressed_sdf_workflow.md`.

v1.8.0-alpha recommendation is experimental and deterministic. It can suggest
adaptive or compressed parameters, but it is not a universal trained model, not
fully trained, and not an optimality guarantee.

v1.12.0-alpha adds optional CPU BVH-accelerated builder sampling. Use
`--accel brute` to keep the original reference path or `--accel bvh` to use
TriangleBVH distance/sign sampling.

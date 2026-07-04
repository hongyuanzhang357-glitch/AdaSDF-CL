# STL to Compressed SDF Workflow

v1.7.0-alpha adds a core-free STL-to-compressed-adaptive-block-SDF workflow.

## Recommended Workflow

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_recommend_build model_clean.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --json recommendation.json --emit-command
adasdf_build_compressed_sdf model_clean.stl model_compressed.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --max-rank 8 --accel bvh --threads 4 --report build_report.md --compression-report compression_report.md --quality-report quality_report.md
adasdf_info model_compressed.sdfbin
adasdf_query model_compressed.sdfbin --point 0 0 0
adasdf_collide model_compressed.sdfbin model_compressed.sdfbin --max-contacts 4
```

## Two-Step Workflow

```bash
adasdf_build_adaptive_sdf model_clean.stl model_adaptive.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --report adaptive_report.md
adasdf_compress_adaptive_sdf model_adaptive.sdfbin model_compressed.sdfbin --target-error 1e-3 --max-rank 8 --report compression_report.md --quality-report quality_report.md
```

`adasdf_build_adaptive_sdf --enable-low-rank` also routes through the
compression path, but `adasdf_build_compressed_sdf` is the clearest public
entry point for one-step output.

## Requirements

Signed builds require watertight input. Open meshes can be handled as unsigned
distance fields with the relevant builder option.

## Boundaries

The compressed model can be queried directly on CPU and can be expanded for
CPU/CUDA benchmark workflows. v1.8 adds deterministic build recommendation
through `adasdf_recommend_build`; it is not a universal trained model, not
fully trained, and not an optimality guarantee. Tucker compression, trained
surrogate integration, and GPU-native compressed query remain planned work.

v1.12 adds optional CPU BVH builder sampling for the adaptive build stage of
the compressed workflow. It does not change Matrix-SVD compression math and is
not GPU-native compressed query.

v1.16 adds opt-in hierarchical adaptive sampling for the adaptive build stage:

```bash
adasdf_build_compressed_sdf model_clean.stl model_compressed.sdfbin \
  --accel bvh --sampling hierarchical --quality-guard \
  --coarse-resolution 3 --quality-check-samples 3
```

Near-surface blocks remain exact by default. Prediction is guarded by sampled
quality checks and falls back to exact sampling on failure. Compression and
SDFBin formats are unchanged.

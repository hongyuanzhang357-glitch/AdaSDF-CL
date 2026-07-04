# Tools

## adasdf_capabilities

Status: working in a core-free public build.

Prints the public capability summary, current boundaries, and documentation
links for the public alpha capability set.
In v1.7 it also reports the adaptive octree/block dense builder and
matrix-SVD compressed adaptive block workflow.

```bash
adasdf_capabilities
adasdf_capabilities --verbose
```

This tool does not require CUDA or the existing core.

## adasdf_mesh_check

Status: working in a core-free public build.

Runs STL mesh diagnostics before SDF construction. It reads ASCII or binary STL
and reports counts, AABB/scale, watertight status, boundary edges,
non-manifold edges, degenerate triangles, duplicate triangles, connected
components, isolated vertices, readiness score, issue severity, and repair
suggestions.

```bash
adasdf_mesh_check model.stl
adasdf_mesh_check model.stl --out mesh_report.md
adasdf_mesh_check model.stl --json mesh_report.json --tolerance 1e-12
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_check model.stl --readiness --strict
adasdf_mesh_check model.stl --readiness --lenient
adasdf_mesh_check model.stl --readiness --clean-out cleaned.stl --clean-report cleanup_report.md
```

This is a preflight diagnostic and readiness tool. With `--clean-out`, it can
run the safe cleanup pass and write a separate cleaned STL. It does not fill
holes or repair self-intersections.

## adasdf_mesh_clean

Status: working in a core-free public build.

Runs safe STL cleanup and writes a separate cleaned ASCII STL plus an optional
before/after Markdown report.

```bash
adasdf_mesh_clean input.stl output_cleaned.stl
adasdf_mesh_clean input.stl output_cleaned.stl --report cleanup_report.md
adasdf_mesh_clean input.stl output_cleaned.stl --merge-tolerance 1e-12 --area-eps 1e-14
```

Cleanup can merge near-duplicate vertices, remove degenerate triangles, remove
duplicate triangles, and remove unused vertices. It refuses to overwrite the
input path and does not perform hole filling, self-intersection repair, boolean
reconstruction, or scale changes.

## adasdf_recommend_demo

Status: working in a core-free public build.

Runs the experimental v0.9 demo surrogate recommender.

```bash
adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
```

The recommender is not universal, not fully trained, and not an optimality guarantee.

## adasdf_benchmark_batch_query

Status: working in CPU mode; CUDA rows run only when the optional CUDA backend is available.

Runs deterministic batch signed-distance, gradient, and normal benchmarks for 10k/100k/1M points.

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --backend cpu,cuda --out benchmark.csv
```

When CUDA is unavailable, the tool prints `CUDA backend unavailable; skipping GPU benchmark.` and keeps CPU benchmark rows.

## adasdf_build_dense_sdf

Status: working in a core-free public build.

Builds a uniform DenseSDF `.sdfbin` from STL input.

```bash
adasdf_build_dense_sdf model.stl model_dense.sdfbin --resolution 64 --padding 0.05
adasdf_build_dense_sdf open_mesh.stl open_mesh_dense.sdfbin --resolution 64 --unsigned
adasdf_build_dense_sdf model.stl model_dense.sdfbin --auto-clean --report dense_report.md --json dense_report.json
```

Signed mode requires watertight input by default. The output format is
`ADASDF_DENSE_SDFBIN_V1`. The builder is a brute-force uniform-grid baseline,
not the adaptive block or low-rank builder.

## adasdf_build_adaptive_sdf

Status: working in a core-free public build.

Builds an adaptive octree/block SDF `.sdfbin` from STL input. Blocks store
dense phi values unless `--enable-low-rank` is requested.

```bash
adasdf_build_adaptive_sdf model.stl model_adaptive.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --report adaptive_report.md
adasdf_build_adaptive_sdf open_mesh.stl open_mesh_adaptive.sdfbin --unsigned --max-level 4
adasdf_build_adaptive_sdf model.stl model_adaptive.sdfbin --dry-run --report adaptive_plan.md
adasdf_build_adaptive_sdf model.stl model_compressed.sdfbin --enable-low-rank --target-compression-error 1e-3 --max-rank 8
```

The default output format is `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`.
`--enable-low-rank` emits `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`. The dedicated
one-step compressed workflow is `adasdf_build_compressed_sdf`.

## adasdf_compress_adaptive_sdf

Status: working in a core-free public build.

Compresses an adaptive block `.sdfbin` into a matrix-SVD compressed adaptive
block `.sdfbin` with optional reports.

```bash
adasdf_compress_adaptive_sdf model_adaptive.sdfbin model_compressed.sdfbin --target-error 1e-3 --max-rank 8
adasdf_compress_adaptive_sdf model_adaptive.sdfbin model_compressed.sdfbin --fixed-rank 4 --report compression_report.md --json compression_report.json --quality-report quality_report.md
```

The output format is `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`. Blocks may remain
dense fallback blocks when compression cannot satisfy the requested error
target.

## adasdf_build_compressed_sdf

Status: working in a core-free public build.

Builds an STL directly into compressed adaptive block SDF output:

```bash
adasdf_build_compressed_sdf model.stl model_compressed.sdfbin --target-error 1e-3 --max-level 5 --block-resolution 8 --max-rank 8 --report build_report.md --compression-report compression_report.md --quality-report quality_report.md
adasdf_build_compressed_sdf model.stl model_compressed.sdfbin --sampling hierarchical --coarse-resolution 3 --quality-check-samples 3 --target-sampling-error 1e-3
```

This command does not implement Tucker/HOSVD, surrogate-guided parameter
recommendation, or GPU-native compressed query.

## adasdf_benchmark_hierarchical_sampling

Status: experimental speed/quality benchmark.

Compares exact adaptive block sampling with hierarchical adaptive sampling.
The benchmark reports build time, speedup, max/mean/RMS/p95 error, sign
mismatches, exact/predicted sample counts, fallback block count, and quality
gate status.

```bash
adasdf_benchmark_hierarchical_sampling model.stl --max-level 2 --block-resolution 5 --coarse-resolution 2 --quality-check-samples 2 --comparison-samples 3 --csv hier.csv --report hier.md --json hier.json
```

Near-surface blocks remain exact by default. Predicted transition/far-field
blocks require the quality guard and fall back to exact sampling on rejection.

## adasdf_sweep_hierarchical_sampling

Status: experimental hierarchical sampling parameter sweep.

Runs `adasdf_benchmark_hierarchical_sampling`-style comparisons over a small
parameter grid and reports whether any quality-preserving configuration can
claim effective acceleration.

```bash
adasdf_sweep_hierarchical_sampling model.stl --max-level 4 --block-resolution 8 --coarse-resolution 2,3,4 --transition-quality-check-samples 1,2,3 --far-field-quality-check corners,sparse --far-field-safety-factor 2.0,3.0 --csv sweep.csv --report sweep.md
```

Speedup claims remain valid only when `quality_gate_passed=true` and
`speedup > 1`.

## adasdf_build_adaptive_sdf_preview

Status: dry-run planning tool.

Prints adaptive builder options and stage status. Octree/block dense
construction is implemented in v1.6 through `adasdf_build_adaptive_sdf`;
matrix-SVD low-rank compression is implemented in v1.7 through
`adasdf_build_compressed_sdf`.

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

This command does not generate `.sdfbin` files.

## adasdf_build_demo_adaptive

Status: working in a core-free public build.

Builds a demo adaptive analytic box `.sdfbin` with octree, block, and rank metadata.

```bash
adasdf_build_demo_adaptive cube_adaptive.sdfbin --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
```

## adasdf_collide_boxes_demo

Status: working in a core-free public build.

Builds two demo adaptive boxes in memory, collides them, enforces `--max-contacts`, and optionally writes an SVG view.

```bash
adasdf_collide_boxes_demo --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view collision.svg
```

## adasdf_make_demo_box

Creates the simpler v0.8 analytic demo `.sdfbin`.

```bash
adasdf_make_demo_box cube_demo.sdfbin
```

## adasdf_build

Status: working when the existing adaptive builder bridge is available.

Builds an STL into the full adaptive compressed `.sdfbin` format and validates the output by reloading it.

```bash
adasdf_build input.stl output.sdfbin --near-surface-error 1e-4 --max-memory-mb 256 --compress
```

## adasdf_info

Prints model metadata, format, shape information, AABB, memory footprint,
DenseSDF metadata, AdaptiveBlockSDF metadata, adaptive demo metadata, format
version, and query backend availability.

```bash
adasdf_info cube_adaptive.sdfbin
```

## adasdf_query

Samples one world-space point and prints signed distance, gradient, normal, and backend.

```bash
adasdf_query cube_adaptive.sdfbin --point 0 0 0
```

## adasdf_expansion_quality

Audits expanded SDF query quality against the model query path and reports
absolute error, sign mismatch, near-surface mismatch, and fallback statistics.

```bash
adasdf_expansion_quality cube_adaptive.sdfbin --expansion global --global-resolution 32 --samples 1000 --out quality.csv
```

## adasdf_collide

Runs a two-object collision query and reports requested/returned contact counts.

```bash
adasdf_collide object_a.sdfbin object_b.sdfbin --offset 0.25 0 0 --max-contacts 8
```

## CollisionWorld tools

Status: working in a core-free public build.

These v1.14 tools load a simple CSV scene of SDF objects, run deterministic
AABB broadphase, evaluate sample-based sparse SDF collision, and optionally
export solver-ready contact candidates.

```bash
adasdf_world_broadphase scene.csv --out pairs.csv --report broadphase.md --json broadphase.json
adasdf_world_sparse_collide scene.csv --threshold 0 --early-exit --out hits.csv --report sparse.md
adasdf_world_solver_contacts scene.csv --threshold 1e-3 --top-k 32 --max-contacts 8 --out contacts.csv --report contacts.md
adasdf_benchmark_collision_world scene.csv --mode sparse --repeat 10 --csv collision_world_benchmark.csv
```

`adasdf_world_sparse_collide` returns code `10` when collision is detected.
That is a successful collision status, not a process failure.

CollisionWorld narrowphase is sample-based SDF collision. It is not exact
mesh-vs-mesh contact, not FCL fallback, not CCD, and not a contact solver.

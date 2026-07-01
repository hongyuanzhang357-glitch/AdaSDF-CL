# Tools

## adasdf_capabilities

Status: working in a core-free public build.

Prints the public capability summary, current boundaries, and documentation
links for the v1.1.1 capability exposure release.

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
holes, repair self-intersections, or build a full arbitrary-STL adaptive SDF.

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
not the full adaptive octree/block/low-rank builder.

## adasdf_build_adaptive_sdf_preview

Status: dry-run interface preview.

Prints the planned adaptive builder options and stages for future
octree/block/low-rank construction.

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

This command does not generate adaptive compressed `.sdfbin` files in
v1.5.0-alpha.

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
DenseSDF metadata, adaptive demo metadata, format version, and query backend
availability.

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

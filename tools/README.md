# Tools

## adasdf_recommend_demo

Status: working in a core-free public build.

Runs the experimental v0.9 demo surrogate recommender.

```bash
adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
```

The recommender is not universal, not fully trained, and not an optimality guarantee.

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

Prints model metadata, format, shape information, AABB, memory footprint, adaptive demo metadata, format version, and query backend availability.

```bash
adasdf_info cube_adaptive.sdfbin
```

## adasdf_query

Samples one world-space point and prints signed distance, gradient, normal, and backend.

```bash
adasdf_query cube_adaptive.sdfbin --point 0 0 0
```

## adasdf_collide

Runs a two-object collision query and reports requested/returned contact counts.

```bash
adasdf_collide object_a.sdfbin object_b.sdfbin --offset 0.25 0 0 --max-contacts 8
```

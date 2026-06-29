# Tools

## adasdf_make_demo_box

Status: working in a core-free public build.

Creates a small text demo `.sdfbin` containing an analytic axis-aligned box.

```bash
adasdf_make_demo_box cube_demo.sdfbin
adasdf_make_demo_box generated/cube_demo.sdfbin --center 0 0 0 --half-extent 0.5 0.5 0.5 --unit m
```

Expected output includes:

```text
AdaSDF-CL demo box generator
Shape: box
Backend: core-free analytic demo SDF backend
Write status: success
Reload validation: success
```

## adasdf_build

Status: working when the existing adaptive builder bridge is available.

Builds an STL into the full adaptive compressed `.sdfbin` format and validates the output by reloading it.

```bash
adasdf_build input.stl output.sdfbin --near-surface-error 1e-4 --max-memory-mb 256 --compress
```

Useful options:

- `--near-surface-error <value>`
- `--max-memory-mb <value>`
- `--block-expand-limit-mb <value>`
- `--max-octree-level <value>`
- `--max-rank <value>`
- `--compress`
- `--no-compress`
- `--unit <name>`
- `--use-surrogate`
- `--top-k <value>`
- `--verbose`

If `--use-surrogate` is requested in the current alpha, the tool reports that the surrogate backend is unavailable and falls back to user-specified build options.

## adasdf_info

Status: working for demo `.sdfbin` files and compatible existing-core `.sdfbin` files.

Prints model metadata, format, shape information when available, AABB, memory footprint, format version, and query backend availability.

```bash
adasdf_info cube_demo.sdfbin
```

Demo output includes:

```text
Format: ADASDF_DEMO_SDFBIN_V1
Shape: box
Query backend: core-free analytic demo SDF backend
AABB min: -0.5 -0.5 -0.5
AABB max: 0.5 0.5 0.5
```

## adasdf_query

Status: working when the loaded `.sdfbin` has an available CPU query backend.

Samples one world-space point and prints signed distance, gradient, normal, and backend.

```bash
adasdf_query cube_demo.sdfbin --point 0 0 0
```

## adasdf_collide

Status: working when both input `.sdfbin` files have compatible metadata and the CPU narrow-phase can sample them.

Runs a two-object collision query with optional translation for object B.

```bash
adasdf_collide cube_demo.sdfbin cube_demo.sdfbin --offset 0.25 0 0 --max-contacts 4
```

Output includes collision state, minimum distance, backend, method, candidate points, raw contacts, reduced contacts, contact count, and first contact details when available.

## Planned Tools

- `adasdf_benchmark`

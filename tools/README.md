# Tools

## adasdf_build

Status: working when the existing adaptive builder bridge is available.

Builds an STL into `.sdfbin` and validates the output by reloading it.

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

Status: working for compatible `.sdfbin` files.

Prints model metadata, AABB, memory footprint, format version, and query backend
availability.

```bash
adasdf_info model.sdfbin
```

## adasdf_query

Status: working when the loaded `.sdfbin` has an available CPU query backend.

Samples one world-space point and prints signed distance, finite-difference
gradient, and normal.

```bash
adasdf_query model.sdfbin --point 0.5 0.5 0.5
```

## adasdf_collide

Status: working when both input `.sdfbin` files have compatible metadata and the
CPU narrow-phase can sample them.

Runs a two-object collision query with optional translation for object B.

```bash
adasdf_collide object_a.sdfbin object_b.sdfbin --offset 0.01 0 0 --max-contacts 16
```

## Planned Tools

- `adasdf_benchmark`

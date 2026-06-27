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

## Planned Tools

- `adasdf_info`
- `adasdf_query`
- `adasdf_collide`
- `adasdf_benchmark`

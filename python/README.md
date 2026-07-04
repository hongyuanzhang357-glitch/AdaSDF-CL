# AdaSDF-CL Python CLI Wrapper

`adasdf_cli` is a lightweight, pure-Python wrapper around installed AdaSDF-CL
command-line tools. It is subprocess-based. It is not a native pybind11 binding,
not a C++ extension module, and not a replacement for the C++ API.

## Requirements

- Python 3.9 or newer.
- Installed or built AdaSDF-CL CLI tools.
- No required runtime dependencies beyond the Python standard library.

## Use With PYTHONPATH

```bash
set ADASDF_BIN=C:\path\to\adasdf-cl\install\bin
set PYTHONPATH=C:\path\to\AdaSDF-CL\python
```

On Linux/macOS:

```bash
export ADASDF_BIN=/path/to/adasdf-cl/install/bin
export PYTHONPATH=/path/to/AdaSDF-CL/python
```

You can also pass `bin_dir=` to each helper.

## Editable Install

Editable install is optional:

```bash
python -m pip install -e python
```

## Example

```python
import adasdf_cli as adasdf

adasdf.mesh_check("model.stl", readiness=True, out="mesh_report.md")

rec = adasdf.recommend_build(
    "model.stl",
    target_error=1e-3,
    memory_mb=512,
    use_case="contact",
    out="recommendation.md",
)
print(rec.recommended_command)

adasdf.build_compressed_sdf(
    "model.stl",
    "model_compressed.sdfbin",
    target_error=1e-3,
    max_level=4,
    block_resolution=8,
    max_rank=8,
)

q = adasdf.query("model_compressed.sdfbin", point=[0, 0, 0])
print(q.phi)

hit = adasdf.sparse_collide(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=0.0,
    early_exit=True,
)
print(hit.colliding)

cand = adasdf.contact_candidates(
    "model_compressed.sdfbin",
    "samples.csv",
    top_k=8,
    threshold=1e-3,
    reduction_radius=0.02,
)
print(cand.candidate_count)

solver_contacts = adasdf.solver_contact_candidates(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=1e-3,
    top_k=32,
    max_contacts=8,
    patch_radius=0.02,
    out="solver_contacts.csv",
)
print(solver_contacts.solver_contact_count)

world_hits = adasdf.world_sparse_collide(
    "scene.csv",
    threshold=0.0,
    early_exit=True,
)
print(world_hits.colliding)
```

## Dry Run

Every helper supports `dry_run=True`. The command is returned without executing:

```python
cmd = adasdf.build_compressed_sdf(
    "model.stl",
    "model.sdfbin",
    target_error=1e-3,
    dry_run=True,
)
print(cmd.command_result.stdout)
```

## Error Handling

All helpers return dataclasses that preserve `stdout`, `stderr`, `returncode`,
and the command list. With the default `check=True`, non-zero CLI exits raise
`AdaSDFCommandError` containing the command, return code, and output previews.
Use `check=False` to inspect failed commands manually.
`sparse_collide()` treats return code `10` as successful collision detection,
not as a process failure.
`world_sparse_collide()` follows the same convention.

## API Surface

- `capabilities`
- `mesh_check`
- `mesh_clean`
- `recommend_build`
- `build_dense_sdf`
- `build_adaptive_sdf`
- `compress_adaptive_sdf`
- `build_compressed_sdf`
- `info`
- `query`
- `collide`
- `sparse_query`
- `sparse_collide`
- `contact_candidates`
- `stabilize_contacts`
- `solver_contact_candidates`
- `expansion_quality`
- `benchmark_batch_query`
- `benchmark_sparse_query`
- `benchmark_contact_reduction`
- `world_broadphase`
- `world_sparse_collide`
- `world_solver_contacts`
- `benchmark_collision_world`
- `recommend_then_build_compressed`
- `preprocess_and_build_compressed`

## Limitations

This wrapper requires installed AdaSDF-CL CLI tools and follows their command
line behavior. It is not a native Python binding and does not expose in-process
C++ objects. `adasdf.query()` wraps the current single-point `adasdf_query`
CLI, so backend, expansion, block selection, and output-mode controls are not
accepted there yet. Use `benchmark_batch_query()` for batch-query benchmark
CLI coverage. Native pybind11 bindings and a C API remain planned work.

# Python CLI Wrapper

AdaSDF-CL v1.8.1-alpha adds `python/adasdf_cli`, a lightweight pure-Python
wrapper around installed AdaSDF-CL command-line tools.

The wrapper is subprocess-based. It is not pybind11, not a native Python
binding, and not a C++ extension module. It requires AdaSDF-CL CLI tools to be
built or installed first.

## Setup

Use the source-tree package directly:

```bash
set ADASDF_BIN=C:\path\to\install\bin
set PYTHONPATH=C:\path\to\AdaSDF-CL\python
```

On Linux/macOS:

```bash
export ADASDF_BIN=/path/to/install/bin
export PYTHONPATH=/path/to/AdaSDF-CL/python
```

You can also pass `bin_dir=` to each helper.

Editable install is optional:

```bash
python -m pip install -e python
```

## API List

- `capabilities`
- `mesh_check`
- `mesh_clean`
- `recommend_build`
- `build_dense_sdf`
- `build_adaptive_sdf`
- `compress_adaptive_sdf`
- `build_compressed_sdf`
- `benchmark_builder_acceleration`
- `info`
- `query`
- `collide`
- `sparse_query`
- `sparse_collide`
- `contact_candidates`
- `stabilize_contacts`
- `solver_contact_candidates`
- `benchmark_contact_reduction`
- `world_broadphase`
- `world_sparse_collide`
- `world_solver_contacts`
- `benchmark_collision_world`
- `select_active_blocks`
- `active_block_query`
- `expansion_quality`
- `benchmark_batch_query`
- `benchmark_builder_acceleration`
- `benchmark_sparse_query`
- `benchmark_block_cache`
- `benchmark_block_lookup`
- `benchmark_hierarchical_sampling`
- `recommend_then_build_compressed`
- `preprocess_and_build_compressed`

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
    accel="bvh",
    threads=4,
)

bench = adasdf.benchmark_builder_acceleration(
    "model.stl",
    builder="compressed",
    accel="bvh",
    threads=4,
    benchmark_brute_reference=True,
)
print(bench.metrics)

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

bench_contacts = adasdf.benchmark_contact_reduction(
    "model_compressed.sdfbin",
    "samples.csv",
    repeat=2,
    warmup=1,
    csv="contact_reduction.csv",
)
print(bench_contacts.metrics)

world_hit = adasdf.world_sparse_collide(
    "scene.csv",
    threshold=0.0,
    early_exit=True,
)
print(world_hit.colliding)

active = adasdf.active_block_query(
    "model_compressed.sdfbin",
    "samples.csv",
    threshold=0.0,
    selection_band=1e-3,
    lookup="hash",
    cache_lookup="spatial-hash",
    report_lookup_stats=True,
    out="active_query.csv",
)
print(active.colliding)

lookup_bench = adasdf.benchmark_block_lookup(
    "model_compressed.sdfbin",
    "samples.csv",
    lookup=["linear", "hash", "morton"],
    cache_lookup=["linear", "hash", "spatial-hash"],
    repeat=5,
    csv="block_lookup.csv",
    report="block_lookup.md",
)
print(lookup_bench.metrics.get("speedup_vs_linear"))
```

`solver_contact_candidates()` and `stabilize_contacts()` export
solver-ready candidates, not solver constraints. They do not compute impulses,
friction forces, Jacobian rows, or a contact solution.

## Dry Run

Every helper supports `dry_run=True`, which returns the command preview without
executing the CLI tool:

```python
preview = adasdf.collide("a.sdfbin", "b.sdfbin", max_contacts=4, dry_run=True)
print(preview.command_result.stdout)
```

## Error Handling

Helpers return dataclasses that preserve:

- command;
- return code;
- stdout;
- stderr;
- elapsed time.

`active_block_query` treats return code `10` as success with
`colliding=True`, matching the CLI convention that code `10` means a collision
was detected.

With `check=True`, a non-zero CLI return code raises `AdaSDFCommandError`.
With `check=False`, the result is returned for manual inspection.

`sparse_collide()` is special: CLI return code `10` means collision detected and
is treated as a successful result by the wrapper.

`world_sparse_collide()` follows the same convention for CollisionWorld scenes.

## Limitations

- Requires installed AdaSDF-CL CLI tools.
- Follows CLI process boundaries and file-based workflows.
- Does not expose in-process C++ objects.
- `adasdf.query()` wraps the current single-point `adasdf_query` CLI and does
  not accept backend, expansion, block selection, or output-mode controls yet.
  Use `benchmark_batch_query()` for batch-query benchmark CLI coverage.
- Does not require numpy, scipy, pandas, pytest, pybind11, or compiled Python
  extensions.
- Native Python bindings and a C API remain planned work.

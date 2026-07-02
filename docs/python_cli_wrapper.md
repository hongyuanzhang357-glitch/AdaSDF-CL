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
- `info`
- `query`
- `collide`
- `sparse_query`
- `sparse_collide`
- `contact_candidates`
- `expansion_quality`
- `benchmark_batch_query`
- `benchmark_sparse_query`
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
```

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

With `check=True`, a non-zero CLI return code raises `AdaSDFCommandError`.
With `check=False`, the result is returned for manual inspection.

`sparse_collide()` is special: CLI return code `10` means collision detected and
is treated as a successful result by the wrapper.

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

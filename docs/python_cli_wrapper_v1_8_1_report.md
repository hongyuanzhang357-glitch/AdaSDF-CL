# AdaSDF-CL v1.8.1-alpha Python CLI Wrapper Report

## Goal

v1.8.1-alpha adds a lightweight pure-Python wrapper for installed AdaSDF-CL CLI
tools. It is a subprocess convenience layer, not a native pybind11 binding and
not a C++ extension module.

## Added Files

- `python/adasdf_cli/__init__.py`
- `python/adasdf_cli/config.py`
- `python/adasdf_cli/runner.py`
- `python/adasdf_cli/results.py`
- `python/adasdf_cli/tools.py`
- `python/adasdf_cli/parsers.py`
- `python/adasdf_cli/exceptions.py`
- `python/adasdf_cli/workflows.py`
- `python/README.md`
- `python/pyproject.toml`
- `python/tests/test_runner.py`
- `python/tests/test_tools_command_building.py`
- `python/tests/test_parsers.py`
- `python/tests/test_python_cli_smoke.py`
- `docs/python_cli_wrapper.md`
- `docs/github_release_draft_v1_8_1_alpha.md`
- `docs/python_cli_wrapper_v1_8_1_report.md`

## Modified Files

- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `include/adasdf/version.h`
- `cmake/adasdf_cl_version.cmake`
- `docs/alpha_status.md`
- `docs/roadmap.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/public_positioning.md`
- `scripts/run_install_validation.py`
- `scripts/run_alpha_validation.py`
- `tools/adasdf_capabilities.cpp`
- `reports/install_validation_summary.md`
- `reports/alpha_validation_summary.md`

## Python Wrapper Structure

`config.py` handles tool discovery. `runner.py` executes subprocess commands.
`results.py` defines result dataclasses. `tools.py` exposes public helpers.
`parsers.py` implements best-effort stdout parsing. `workflows.py` composes
helpers into small convenience workflows.

## Tool Discovery

Tool lookup checks explicit `bin_dir`, `ADASDF_BIN`, `ADASDF_CL_BIN`, and then
`PATH`. Windows `.exe` suffixes are handled automatically.

## Supported Functions

The wrapper supports capabilities, mesh check, mesh clean, build
recommendation, DenseSDF build, AdaptiveBlockSDF build, compressed SDF build,
info, query, collide, expansion quality, benchmark, and two convenience
workflows.

## Result Dataclasses

Every result preserves the underlying `CommandResult`, including command,
returncode, stdout, stderr, and elapsed time. Specialized results expose parsed
fields such as recommended command, recommended path, phi, normal, collision
state, contact count, minimum distance, info format, and benchmark metrics.

## Parsers

Parsers are best effort and never raise on format changes. Failed parsing
returns `None` or an empty dictionary while preserving the original command
result.

## Error Handling

With `check=True`, non-zero CLI exits raise `AdaSDFCommandError` containing the
command, return code, stdout preview, and stderr preview. With `check=False`,
the result is returned unchanged.

## Dry-Run Behavior

`dry_run=True` returns a successful command preview without executing any CLI
tool. This supports command inspection and notebook/script planning.

## Validation Results

- Python unittest with real CLI smoke: PASS, 22 tests.
- CPU CTest: PASS, 101/101 tests.
- CUDA configure/build/CTest: PASS, 101/101 tests with CUDA 12.6 using an
  ASCII-only temporary drive mapping.
- Install validation: PASS, including Python CLI Wrapper Tests.
- Alpha validation: PASS, including Python CLI Wrapper Tests and final clean
  check.
- Standalone clean check: PASS.

Validation scripts set `PYTHONDONTWRITEBYTECODE=1` for wrapper tests so the
source tree remains free of `__pycache__` directories during release checks.

## Current Limitations

- Requires installed AdaSDF-CL CLI tools.
- Uses process and file boundaries, not in-process C++ objects.
- `adasdf.query()` wraps the current single-point `adasdf_query` CLI and does
  not accept backend, expansion, block selection, or output-mode controls yet.
- Does not implement pybind11.
- Does not implement a native C++ extension module.
- Does not publish to pip in v1.8.1.

## Next Steps

- Native pybind11 binding.
- C API for stable external FFI.
- GPU-native compressed query.
- FCL hybrid fallback and CollisionWorld broadphase.

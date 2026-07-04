# AdaSDF-CL v1.13.0-alpha.1 CI Hotfix Report

Note: `v1.13.0-alpha.1` was superseded by `v1.13.0-alpha.2` after the
`v1.13.0-alpha.1` tag workflow was cancelled during the Ubuntu build step.

## GitHub Actions Failure Summary

`v1.13.0-alpha` pushed successfully, but both push-triggered CI runs failed on
Ubuntu:

- main run: `28667426951`
- main Ubuntu job: `85022475302`
- tag run: `28667424713`
- tag Ubuntu job: `85022468050`

Both runs had the same pattern:

- `windows-latest Release`: PASS.
- `ubuntu-latest Release`: Configure PASS, Build PASS, Test PASS.
- `ubuntu-latest Release`: Install validation failed with exit code `143`.

The failing log ended during the install-validation build phase:

```text
[install-validation] Configure
[install-validation] Build
The runner has received a shutdown signal.
Process completed with exit code 143.
```

## Root Cause

The Ubuntu CI job already configured, built, and tested `../build/adasdf_cl_ci`,
then `run_install_validation.py` configured and built a second full source tree
under `../build/adasdf_cl_ci_install_validation`. The expanded v1.13 test/tool
surface made this duplicate Ubuntu build long enough for the runner to receive a
shutdown signal before install validation could finish.

This was not an algorithmic failure, not a C++ compile failure, not a test
failure, and not a Python wrapper failure. The failure happened after CTest had
already passed `153/153`.

## Files Changed

- `.github/workflows/ci.yml`
- `include/adasdf/version.h`
- `cmake/adasdf_cl_version.cmake`
- `python/adasdf_cli/__init__.py`
- `python/pyproject.toml`
- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `docs/alpha_status.md`
- `docs/ci_hotfix_v1_13_0_alpha_1_report.md`
- `scripts/run_install_validation.py`
- `scripts/run_alpha_validation.py`

## Fix Explanation

The workflow now configures the primary CI build with the same feature flags
used by install validation and with the matrix build type. Install validation
then uses `--reuse-build` to reuse the already-built `../build/adasdf_cl_ci`
tree instead of creating a second full source build.

This preserves install validation coverage while avoiding redundant compile
time on Ubuntu. Package-consumption and downstream CMake checks still run from
the installed prefix.

`run_alpha_validation.py --include-install` also passes `--reuse-build` to the
nested install validation step, so local alpha validation no longer performs a
second full source build after its own configure/build/CTest phase.

## Local Validation Results

- CPU-only configure/build: PASS.
- CTest: PASS, `153/153`.
- Install validation with reused CI build: PASS.
- Alpha validation with included install validation: PASS.
- Python unittest: PASS, `39/39`.
- Clean check: PASS.
- `git diff --check`: PASS, with line-ending normalization warnings only.

## Tag Policy

The original `v1.13.0-alpha` tag remains unchanged for traceability.

## New Tag Recommendation

Use `v1.13.0-alpha.2` for the green CI pre-release.

## Release Recommendation

Create the GitHub pre-release from `v1.13.0-alpha.2` after GitHub Actions pass
on both `main` and the new tag.

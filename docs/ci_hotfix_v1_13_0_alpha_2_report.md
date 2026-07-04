# AdaSDF-CL v1.13.0-alpha.2 CI Hotfix Report

## GitHub Actions Failure Summary

Commit `636fa6adfd25438a9910b36c597e1fa5d38b8622` had different outcomes for
the same CI workflow:

- `main` workflow run `28670722067`: PASS.
- `v1.13.0-alpha.1` workflow run `28670723566`: FAIL.
- Tag Windows job `85033128388`: PASS.
- Tag Ubuntu job `85033128390`: FAIL.

The tag Ubuntu job failed in the normal `Build` step. `Configure` passed and
reported `Version: 1.13.0-alpha.1`. `Test`, `Install validation`, and
`Clean repo check` were skipped because the build step was cancelled.

Relevant log excerpt:

```text
Run cmake --build ../build/adasdf_cl_ci --config Release --parallel
...
The runner has received a shutdown signal. This can happen when the runner
service is stopped, or a manually started runner is canceled.
...
The operation was canceled.
```

## Tag Workflow Diagnosis

- No tag-only workflow step was present.
- No release draft, tag validation, or package validation step ran before the
  failure.
- The `v1.13.0-alpha.1` suffix parsed correctly in CMake configure output.
- The Python package version `1.13.0a1` was PEP 440 compatible and was not
  involved in the failure.
- The tag run failed before install validation and alpha validation could run.
- The failure was specific to Ubuntu `Build`; Windows passed.

## Root Cause

The tag workflow used unbounded CMake build parallelism on Ubuntu:

```bash
cmake --build ../build/adasdf_cl_ci --config Release --parallel
```

At v1.13 scale, this starts many compiler jobs across the runtime library,
tools, benchmarks, examples, and 153 test executables. When `main` and tag
workflows are triggered for the same commit, the Ubuntu runner can be cancelled
during this high-load build phase. The successful `main` run shows the source
tree and validation logic are otherwise sound.

This is not an algorithmic failure and not an `alpha.1` version parsing failure.

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
- `docs/ci_hotfix_v1_13_0_alpha_2_report.md`

## Fix Explanation

The CI build step now limits CMake parallelism:

```bash
cmake --build ../build/adasdf_cl_ci --config ${{ matrix.config }} --parallel 2
```

This keeps the same configure options, build targets, CTest coverage, install
validation, and clean check while reducing Ubuntu peak build pressure. The
existing `--reuse-build` install validation path remains unchanged.

## Version Mapping

- AdaSDF-CL version string: `1.13.0-alpha.2`.
- CMake numeric version: `1.13.0`.
- CMake suffix: `alpha.2`.
- Python package version: `1.13.0a2`.

## Tag Policy

The original `v1.13.0-alpha` and `v1.13.0-alpha.1` tags remain unchanged for
traceability. Do not move or force-push those tags.

## Release Recommendation

Use `v1.13.0-alpha.2` for the green CI pre-release after GitHub Actions pass on
both `main` and the new tag.

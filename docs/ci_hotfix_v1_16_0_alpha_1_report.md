# v1.16.0-alpha.1 Tag Hotfix Report

## Original Issue

- At diagnosis time, `main` pointed to `fc80d09`.
- The original `v1.16.0-alpha` tag peeled to `b7915d9`.
- The tag did not include the final v1.16 documentation/report commits.

## Fix

- The original `v1.16.0-alpha` tag was not moved.
- A new hotfix tag, `v1.16.0-alpha.1`, is used for the aligned v1.16 release.
- Version metadata was updated to `1.16.0-alpha.1`.
- Python package metadata was updated to `1.16.0a1`.
- A new GitHub release draft was added for `v1.16.0-alpha.1`.

## Algorithmic Changes

No algorithmic changes are intended in this hotfix. The functional release
remains hierarchical adaptive sampling with quality guard.

## Local Validation

- CPU-only configure/build: PASS.
- CPU CTest: PASS, 184/184 tests.
- Python unittest: PASS, 53 tests.
- Install validation: PASS.
- Alpha validation with install validation: PASS.
- `python scripts/check_repo_clean.py .`: PASS.
- `git diff --check`: PASS.
- Desktop root temporary artifact scan: PASS, no matching generated artifacts
  found.

## Speed-Quality Smoke Summary

Fixture-level `closed_cube_ascii.stl` smoke benchmark results:

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | max_abs_error | rms_error | p95_error | sign_mismatch_count | near_surface_sign_mismatch_count | quality_gate_passed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| cube L4 | 908.183 | 1535.82 | 0.591335 | 0 | 0 | 0 | 0 | 0 | true |
| cube L5 | 6850.62 | 21061.0 | 0.325275 | 0 | 0 | 0 | 0 | 0 | true |

These smoke results pass quality checks but do not demonstrate effective
speedup on the small cube fixture. The overhead of hierarchical prediction and
quality checks exceeds exact-sampling savings for these cases.

## Release Recommendation

Use `v1.16.0-alpha.1` for the GitHub pre-release. Keep `v1.16.0-alpha`
unchanged for traceability.

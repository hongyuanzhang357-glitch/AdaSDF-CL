# AdaSDF-CL v1.16.0-alpha.3 Release Report

## Summary

This report records the local release finalization for
`v1.16.0-alpha.3`. The release promotes the contact-focused narrow-band SDF
construction work from the local performance branch into `main` as an alpha,
opt-in, collision-oriented construction path.

This release does not claim global full-field SDF reconstruction acceleration.
The public performance language is limited to end-to-end contact-band
benchmarks whose quality, coverage and timing gates pass.

## Merged Local Commits

The following local commits were fast-forward merged from
`perf/contact-focused-narrow-band-local` into `main`:

- `933f578 Optimize hierarchical exact hot path locally`
- `4bae8bb Prototype contact-focused narrow-band sampling locally`
- `7c69f7d Reduce contact-band marker overmarking locally`
- `d2c4792 Harden contact-focused narrow-band sampling locally`
- `ad5e25f Clarify contact-band timing semantics locally`

The release commit updates version metadata, documentation and builder CLI
parsing for `v1.16.0-alpha.3`.

## Version Metadata

- C++ version string: `1.16.0-alpha.3`
- CMake package suffix: `alpha.3`
- Python wrapper version: `1.16.0-alpha.3`
- Python package version: `1.16.0a3`
- Citation version: `1.16.0-alpha.3`
- Release date: `2026-07-06`

## Added Capability

`v1.16.0-alpha.3` documents and exposes contact-focused narrow-band sampling:

- opt-in `--sampling contact-band` builder path;
- distance-aware and hybrid contact-band marker support;
- local halo mode with far-field interpolation;
- contact-band phi/sign/normal quality audit;
- contact-band coverage audit;
- marker cost audit;
- release-facing end-to-end timing semantics;
- performance claim gate;
- synthetic thin-gap, robot-like and dense curved mesh fixtures;
- `adasdf_benchmark_contact_band_sampling`;
- `adasdf_sweep_contact_band_sampling`;
- Python wrapper support for `benchmark_contact_band_sampling`.

The default builder path remains exact.

## Timing Semantics

The public metric is:

```text
speedup_end_to_end =
    exact_reference_wall_time_ms / contact_band_wall_time_ms
```

Release-facing speedup includes marker cost and audit time. The benchmark still
reports diagnostic-only views such as marker-excluded speedup, audit-excluded
speedup and diagnostics-excluded speedup, but those are not headline release
claims.

`performance_claim_allowed` is true only when the end-to-end speedup is above
1, quality passes, coverage passes, sign mismatches are zero, normal flips are
zero and the timing scope includes marker and audit work.

## Release Smoke Benchmark

The final local smoke benchmark used the project-generated
`closed_cube_ascii.stl` fixture:

```text
adasdf_benchmark_contact_band_sampling tests/data/mesh_diagnostics/closed_cube_ascii.stl
  --max-level 5
  --block-resolution 8
  --target-error 1e-3
  --contact-band-width 5e-4
  --contact-band-layers 1
  --halo-exact-layers 1
  --contact-band-marker distance-aware
  --marker-cell-size-factor 0.5
  --marker-safety-factor 1.0
  --local-halo-only
  --far-field-resolution 3
  --far-field-mode coarse-interpolate
  --reuse-far-field-sign
  --normal-audit
  --coverage-audit
  --coverage-samples-per-axis 3
  --marker-cost-audit
  --timing-mode end-to-end
  --include-audit-in-wall-time
  --include-marker-in-speedup
  --threads 2
```

Result:

| metric | value |
| --- | ---: |
| exact_reference_wall_time_ms | 18149.8 |
| contact_band_wall_time_ms | 12804.1 |
| speedup_end_to_end | 1.41750 |
| speedup_core_build | 1.44547 |
| speedup_excluding_marker | 4.16921 |
| exact_node_ratio | 0.0866263 |
| contact_band_block_ratio | 0.173304 |
| contact_band_quality_passed | true |
| coverage_passed | true |
| performance_claim_allowed | true |

The marker-excluded value is diagnostic only. The release-facing result for
this smoke run is `speedup_end_to_end = 1.41750`.

## Validation Results

Local validation was run in an out-of-tree temporary workspace so generated
build, install, benchmark and report artifacts stayed outside the repository.

Results:

- CPU-only Release build: PASS
- CTest: 208/208 PASS
- install validation: PASS
- alpha validation with install: PASS
- Python wrapper unittest: 57/57 PASS
- repository clean check: PASS
- `git diff --check`: PASS

## Limitations

- Contact-band sampling is alpha and opt-in.
- Far-field phi is relaxed and should not be interpreted as globally
  high-accuracy SDF reconstruction.
- Performance is fixture- and parameter-dependent.
- No external models or unknown-license STL files were used.
- No `.sdfbin`, CSV, log, build or install outputs are part of the release
  commit.

## Release Recommendation

Create the GitHub pre-release for `v1.16.0-alpha.3` only after GitHub Actions
are green for both `main` and the `v1.16.0-alpha.3` tag. Keep all older
`v1.16.0-alpha*` tags untouched.

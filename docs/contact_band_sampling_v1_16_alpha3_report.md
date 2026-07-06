# AdaSDF-CL v1.16.0-alpha.3 Contact-Band Sampling Report

## Summary

AdaSDF-CL v1.16.0-alpha.3 formalizes the local contact-focused narrow-band
SDF construction work as an alpha, opt-in, collision-oriented construction
path.

The feature exact-samples the contact band, fills relaxed far-field values by
coarse interpolation or a bounded far-field policy, audits contact-band phi,
sign and normal quality, audits coverage of contact-band points, reports marker
cost, and separates end-to-end timing from diagnostic-only timing views.

This is not a global full-field SDF reconstruction acceleration claim.

## Integrated Local Sprints

The formal alpha.3 report integrates the local contact-band sprint series:

- contact-focused narrow-band sampling prototype;
- distance-aware marker and over-marking reduction;
- synthetic fixture hardening for thin gaps, robot-like links, dense curved
  meshes and gear-like teeth;
- timing semantics and performance claim gating.

The local reports are retained for traceability:

- `docs/local_contact_band_sampling_report.md`;
- `docs/local_contact_band_marker_overmarking_report.md`;
- `docs/local_contact_band_release_hardening_report.md`;
- `docs/local_contact_band_timing_readiness_report.md`.

## CLI And Builder Readiness

The default builder path remains exact. Contact-band sampling is enabled only
when explicitly requested.

Adaptive builder:

```bash
adasdf_build_adaptive_sdf model.stl model_contact_band.sdfbin \
  --sampling contact-band \
  --contact-band-width 5e-4 \
  --contact-band-layers 1 \
  --halo-exact-layers 1 \
  --contact-band-marker distance-aware \
  --marker-cell-size-factor 0.5 \
  --marker-safety-factor 1.0 \
  --local-halo-only \
  --far-field-resolution 3 \
  --far-field-mode coarse-interpolate \
  --reuse-far-field-sign \
  --audit contact-band \
  --contact-band-normal-audit
```

Compressed builder:

```bash
adasdf_build_compressed_sdf model.stl model_contact_band_compressed.sdfbin \
  --sampling contact-band \
  --contact-band-marker distance-aware \
  --contact-band-width 5e-4 \
  --contact-band-layers 1 \
  --halo-exact-layers 1 \
  --far-field-resolution 3 \
  --far-field-mode coarse-interpolate \
  --reuse-far-field-sign \
  --audit contact-band \
  --contact-band-normal-audit
```

Benchmark and sweep tools:

- `adasdf_benchmark_contact_band_sampling`;
- `adasdf_sweep_contact_band_sampling`.

## Timing Semantics

The release-facing timing metric is `speedup_end_to_end`:

```text
speedup_end_to_end =
    exact_reference_wall_time_ms / contact_band_wall_time_ms
```

Default benchmark semantics:

- timing mode: `end-to-end`;
- audit included in wall time;
- marker included in speedup;
- marker-excluded speedup is diagnostic only;
- audit-excluded speedup is diagnostic only;
- diagnostics-excluded speedup is diagnostic only.

`performance_claim_allowed` is true only when:

- `speedup_end_to_end > 1`;
- contact-band quality passes;
- coverage passes;
- contact-band sign mismatches are zero;
- near-surface sign mismatches are zero;
- normal flips are zero;
- timing mode is end-to-end;
- marker and audit are included in the release-facing timing scope.

## Final Local Timing Results

All fixtures below are deterministic, project-generated synthetic STL fixtures.

| Case | speedup_end_to_end | speedup_core_build | speedup_excluding_marker | exact_node_ratio | quality | coverage | performance_claim_allowed |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| cube L5 | 1.41750 | 1.44547 | 4.16921 | 0.0866263 | PASS | PASS | yes |
| wavy L4 | 1.34531 | 1.35695 | 4.79138 | 0.123155 | PASS | PASS | yes |
| thin gap | 2.37600 | 2.38693 | 6.09417 | 0.0904199 | PASS | PASS | yes |
| robot link | 2.36958 | 2.38165 | 5.57235 | 0.0942893 | PASS | PASS | yes |
| dense wavy | 2.83501 | 2.84420 | 3.18525 | 0.180487 | PASS | PASS | yes |

The end-to-end speedup range on the tested synthetic collision-oriented
fixtures is `1.34531x` to `2.83501x`. The cube L5 row is the release smoke run
for this candidate. The marker-excluded speedup range is larger, but it is
diagnostic-only and must not be used as a headline claim.

## Quality Boundary

The contact-band quality gate protects the collision-relevant region:

- contact-band phi error;
- sign consistency;
- near-surface sign consistency;
- normal angle error;
- normal flip checks;
- coverage of contact-band points.

Far-field phi is intentionally relaxed. This is appropriate for
collision-oriented construction and rejection, but it is not a guarantee of
globally high-accuracy full-field SDF reconstruction.

## Python Wrapper

The pure-Python CLI wrapper exposes:

- `benchmark_contact_band_sampling(...)`;
- contact-band width, layers, halo, marker, marker cell factor, marker safety,
  local halo, far-field resolution, far-field mode, far-field sign reuse,
  normal audit, coverage audit, timing mode, CSV/report output, case id and
  dry-run arguments;
- parser fields for `speedup_end_to_end`, `speedup_core_build`,
  `speedup_excluding_marker`, `performance_claim_allowed`,
  `contact_band_quality_passed`, `coverage_passed` and
  `p95_normal_angle_error_deg`.

`adasdf_sweep_contact_band_sampling` is a C++ CLI tool. The Python wrapper does
not currently expose a separate contact-band sweep helper.

## Recommended Public Wording

Allowed:

- "collision-oriented contact-band construction acceleration";
- "end-to-end contact-band benchmark speedup";
- "quality-gated and coverage-gated contact-band speedup";
- "project-generated synthetic collision-oriented fixtures."

Avoid:

- "universal speedup for all STL models";
- "global full-field SDF reconstruction acceleration";
- "marker-excluded speedup" as end-to-end performance;
- "industrial certification."

## Recommendation

v1.16.0-alpha.3 is ready to be promoted as an alpha pre-release if local
validation passes and GitHub Actions are green for both `main` and the
`v1.16.0-alpha.3` tag.

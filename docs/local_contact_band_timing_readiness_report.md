# Local Contact-Band Timing Readiness Report

## Goal

This local sprint clarifies contact-band benchmark timing semantics before any
public release promotion. It does not change the public version string, create a
tag, move a tag, or push remote state.

The main goal is to make the default performance claim explicitly end-to-end:
the default wall time includes contact-band marker work and quality audit work,
and the default speedup includes marker work. Diagnostic speedups that exclude
marker, audit, or diagnostics are reported only to explain bottlenecks.

## Why the Timing Split Is Needed

Earlier contact-band reports mixed accumulated per-block marker timings with the
benchmark wall clock. That made the marker cost visible, but it also made it too
easy to quote an excluding-marker number as if it were the full collision-facing
runtime. This sprint separates:

- exact reference wall time;
- contact-band wall time;
- exact reference core build time;
- contact-band core build time;
- marker, sampling, interpolation, audit, report, and diagnostics timing fields;
- diagnostic-only excluding-marker, excluding-audit, and excluding-diagnostics
  speedups.

The only release-facing speedup in this report is `speedup_end_to_end`.

## Benchmark Setup

All cases were run locally from the CPU-only Release build in the ASCII-only
workspace. Results were written to the temporary work root, not to the desktop
root and not to committed benchmark artifacts.

Common settings:

- `--contact-band-marker distance-aware`;
- `--local-halo-only`;
- `--normal-audit`;
- `--coverage-audit`;
- `--marker-cost-audit`;
- `--timing-mode end-to-end`;
- `--include-audit-in-wall-time`;
- `--include-marker-in-speedup`;
- `--threads 1`;
- `--block-resolution 8`;
- `--target-error 1e-3`.

Case-specific settings:

| Case | Fixture | Max level | Marker cell-size factor | Marker safety factor |
| --- | --- | ---: | ---: | ---: |
| timing_cube_L5 | `closed_cube_ascii.stl` | 5 | 0.5 | 1.0 |
| timing_wavy_L4 | `wavy_sphere_ascii.stl` | 4 | 0.5 | 1.0 |
| timing_thin_gap | `thin_gap_box_ascii.stl` | 5 | 0.75 | 1.5 |
| timing_robot_link | `robot_link_like_ascii.stl` | 5 | 0.5 | 1.0 |
| timing_dense_wavy | `dense_wavy_sphere_ascii.stl` | 4 | 0.5 | 1.0 |

## End-to-End Speedup

`speedup_end_to_end` is the release-facing performance metric. It compares
exact reference wall time against contact-band wall time with audit included and
marker included.

| Case | Exact reference wall ms | Contact-band wall ms | Speedup end-to-end | Quality | Coverage | Performance claim allowed |
| --- | ---: | ---: | ---: | --- | --- | --- |
| timing_cube_L5 | 43570.1 | 25851.0 | 1.68543 | PASS | PASS | yes |
| timing_wavy_L4 | 1573.73 | 1169.79 | 1.34531 | PASS | PASS | yes |
| timing_thin_gap | 135402.0 | 56987.7 | 2.37600 | PASS | PASS | yes |
| timing_robot_link | 138367.0 | 58393.3 | 2.36958 | PASS | PASS | yes |
| timing_dense_wavy | 67954.8 | 23969.9 | 2.83501 | PASS | PASS | yes |

All five local fixtures passed the quality gate, coverage gate, sign-mismatch
gate, near-surface sign gate, normal audit, and the new performance claim gate.

## Core-Build Speedup

`speedup_core_build` compares the reference build path against the contact-band
core build path. It excludes the audit wall time and is useful for internal
engineering analysis, but it should not replace `speedup_end_to_end` in external
claims.

| Case | Exact core build ms | Contact-band core build ms | Speedup core build |
| --- | ---: | ---: | ---: |
| timing_cube_L5 | 43570.1 | 25572.0 | 1.70382 |
| timing_wavy_L4 | 1573.73 | 1159.75 | 1.35695 |
| timing_thin_gap | 135402.0 | 56726.5 | 2.38693 |
| timing_robot_link | 138367.0 | 58097.4 | 2.38165 |
| timing_dense_wavy | 67954.8 | 23892.4 | 2.84420 |

The core-build speedup is close to the end-to-end speedup because audit wall
time is currently small relative to build time on these fixtures.

## Excluding-Marker Diagnostic

`speedup_excluding_marker` removes the wall-normalized marker portion from the
contact-band wall time estimate. It is useful for understanding where future
optimization should focus, but it is diagnostic-only and must not be used as a
headline performance claim.

| Case | Speedup end-to-end | Speedup excluding marker | Marker fraction of wall |
| --- | ---: | ---: | ---: |
| timing_cube_L5 | 1.68543 | 4.69458 | 0.640984 |
| timing_wavy_L4 | 1.34531 | 4.79138 | 0.719222 |
| timing_thin_gap | 2.37600 | 6.09417 | 0.610120 |
| timing_robot_link | 2.36958 | 5.57235 | 0.574762 |
| timing_dense_wavy | 2.83501 | 3.18525 | 0.109959 |

The large gap between end-to-end and excluding-marker speedup on cube, wavy,
thin-gap, and robot-link cases shows that marker cost is still a major
optimization target. The dense-wavy case is different: sampling dominates more
of the wall time, so excluding marker changes the result less.

## Audit and Diagnostics Fractions

| Case | Audit fraction of wall | Diagnostics fraction of wall |
| --- | ---: | ---: |
| timing_cube_L5 | 0.0107944 | 0.00637941 |
| timing_wavy_L4 | 0.00857636 | 0.00453972 |
| timing_thin_gap | 0.00458218 | 0.00304561 |
| timing_robot_link | 0.00506763 | 0.00315198 |
| timing_dense_wavy | 0.00323108 | 0.000980376 |

Audit and diagnostics are visible but not dominant on these cases. Keeping
audit in the default wall clock is still the right default because it prevents a
quality-unsafe speedup from becoming the release-facing number.

`contact_band_report_time_ms` is currently emitted as `0` by the benchmark
because CSV and Markdown writing happen after the benchmark result is assembled.
The field exists for future callers that choose to measure report generation
inside their own timing scope.

## Performance Claim Gate

`performance_claim_allowed` is true only when:

- timing mode is `end-to-end`;
- audit is included in wall time;
- marker is included in speedup;
- marker and audit are not explicitly excluded;
- `speedup_end_to_end > 1`;
- quality and coverage both pass;
- contact-band sign mismatches are zero;
- near-surface sign mismatches are zero;
- normal flips and near-surface normal flips are zero.

All five local timing cases passed this gate.

## External Wording

Allowed wording:

- "collision-oriented contact-band construction acceleration";
- "end-to-end contact-band benchmark speedup";
- "quality-gated and coverage-gated contact-band speedup";
- "on the tested local fixtures, end-to-end contact-band speedup ranged from
  1.34531x to 2.83501x."

Not allowed wording:

- "global SDF reconstruction acceleration";
- "marker-excluded speedup" as a release headline;
- "full-domain far-field phi accuracy";
- "universal speedup on arbitrary CAD";
- "excluding-marker speedup" without calling it diagnostic-only.

## Recommended Defaults if Formalized

If this local branch is later promoted as a release candidate, keep these
defaults:

- timing mode: `end-to-end`;
- audit included in wall time;
- marker included in speedup;
- `speedup_end_to_end` as the only external performance metric;
- distance-aware marker;
- local halo for contact-oriented benchmark paths;
- marker cell-size factor `0.5` and marker safety factor `1.0` for general
  cases;
- a conservative thin-gap preset using marker cell-size factor `0.75` and
  marker safety factor `1.5`;
- normal audit and coverage audit available in benchmark/report paths.

## Recommendation

The timing semantics are ready enough to formalize in a future
`v1.16.0-alpha.3` style promotion, provided the release text keeps the claim
limited to end-to-end, collision-oriented contact-band benchmarks.

The next smallest optimization task should target marker cost directly:

1. reduce repeated BVH/AABB marker work;
2. cache or batch marker candidate evaluation;
3. keep reporting both end-to-end speedup and diagnostic excluding-marker
   speedup;
4. never let the excluding-marker value become the external claim metric.

No public version change, tag, or push was performed for this local report.

# Local Contact-Band Release Hardening Report

## Goal

This local sprint hardens contact-focused narrow-band sampling before promoting it beyond the local branch. It focuses on safety, diagnostic visibility, synthetic fixtures, benchmark repeatability, and Python wrapper readiness. No public version string, release tag, or remote state is changed by this report.

## Previous Baseline

The previous local sprint showed that contact-focused narrow-band sampling was effective on the original cube and wavy-sphere cases:

| Case | Previous result |
| --- | ---: |
| cube L5 distance-aware | speedup 2.34612x, quality PASS |
| wavy L4 distance-aware, old marker | 3872 contact blocks, exact-node ratio 0.942491, speedup 0.813745x |
| wavy L4 distance-aware, reduced overmarking | 922 contact blocks, exact-node ratio 0.122179, speedup 1.36031x |
| wavy L4 sweep best | speedup 1.4947x, exact-node ratio 0.107262, quality PASS |

## New Fixtures

The sprint adds deterministic, project-generated ASCII STL fixtures:

| Fixture | Purpose | Mesh diagnostic result |
| --- | --- | --- |
| `thin_gap_box_ascii.stl` | Small-gap and thin-slot contact-band coverage | Watertight, no boundary edges, no degenerate triangles |
| `robot_link_like_ascii.stl` | Robot-link-like collision geometry with multiple linked boxes | Watertight, no boundary edges, no degenerate triangles |
| `dense_wavy_sphere_ascii.stl` | Denser curved surface for marker refinement cost | Watertight, single component, no degenerate triangles |
| `gear_like_teeth_ascii.stl` | High-curvature tooth-like boundary fixture | Watertight, single component, no degenerate triangles |

The fixtures are generated locally by `adasdf_generate_synthetic_fixture` and contain the `project_generated` STL marker. No external model or unknown-license geometry is used.

## Benchmark Summary

All results were produced from the CPU-only local hardening build with distance-aware marker, local halo, normal audit, coverage audit, and marker-cost audit enabled.

| Case | Speedup | Exact-node ratio | Quality | Coverage | P95 normal angle error deg | Sign mismatches | Near-surface sign mismatches | Marker time ms | Marker time fraction |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| hardening_cube_L5 | 1.3738 | 0.0866263 | PASS | PASS | 1.20742e-06 | 0 | 0 | 16670.1 | 0.671221 |
| hardening_wavy_L4 | 1.27382 | 0.122179 | PASS | PASS | 1.70755e-06 | 0 | 0 | 875.805 | 0.740655 |
| hardening_thin_gap | 2.31192 | 0.0904199 | PASS | PASS | 1.20742e-06 | 0 | 0 | 35788.6 | 0.622268 |
| hardening_robot_link | 2.54182 | 0.0924492 | PASS | PASS | 1.20742e-06 | 0 | 0 | 29519.3 | 0.553784 |
| hardening_dense_wavy | 2.89073 | 0.177881 | PASS | PASS | 1.70755e-06 | 0 | 0 | 2572.05 | 0.112873 |

## Coverage Audit

Coverage audit adds a guard that fails quality when exact-reference sampled points inside `|phi| <= contact_band_width` are not covered by the marker mask.

| Case | Coverage checks | Missed contact-band points | Missed contact-band cells | Coverage passed |
| --- | ---: | ---: | ---: | --- |
| hardening_cube_L5 | 0 | 0 | 0 | yes |
| hardening_wavy_L4 | 83 | 0 | 0 | yes |
| hardening_thin_gap | 0 | 0 | 0 | yes |
| hardening_robot_link | 12614 | 0 | 0 | yes |
| hardening_dense_wavy | 386 | 0 | 0 | yes |

The zero-check cases had no sampled audit points classified inside the strict contact-band threshold for the chosen audit grid. They still passed quality with zero contact-band sign mismatches, zero near-surface sign mismatches, and successful normal audit.

## Marker Cost Diagnostics

The marker now reports candidate and refinement costs:

| Case | Candidate cells | Candidate triangles | Refined candidates | Rejected candidates | Accepted contact cells | False-positive proxy | BVH query ms | Box-triangle distance ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hardening_cube_L5 | 247256 | 368457 | 368457 | 121201 | 247256 | 0.328942 | 14132.4 | 409.947 |
| hardening_wavy_L4 | 1170828 | 1982248 | 1982248 | 1898990 | 83258 | 0.957998 | 414.973 | 393.851 |
| hardening_thin_gap | 318160 | 467515 | 467515 | 149355 | 318160 | 0.319466 | 32816 | 525.331 |
| hardening_robot_link | 389160 | 572001 | 572001 | 182841 | 389160 | 0.319652 | 26353.4 | 648.264 |
| hardening_dense_wavy | 217008 | 325053 | 325053 | 197311 | 127742 | 0.607012 | 1762.57 | 414.185 |

`marker_time_ms` is accumulated per block, while `contact_band_time_ms` remains the reliable wall-clock end-to-end total. `marker_time_fraction` is normalized against accumulated block work, not wall-clock time. `effective_speedup_including_marker` is the reliable end-to-end speedup. `effective_speedup_excluding_marker` is a diagnostic estimate that removes the marker fraction from the wall-clock total and should not be used as a release headline metric.

## Failed Cases

No benchmark case failed the local release-hardening gate. All required cases passed quality, coverage, normal audit, and contact-band sign checks.

## Promotion Recommendation

The local data supports formalizing the feature as a future `v1.16.0-alpha.3`, provided the release keeps conservative defaults:

- use distance-aware contact-band marking;
- keep `--local-halo-only` for contact-oriented sampling;
- keep `marker_cell_size_factor` near `0.5` for general cases;
- use a larger `marker_safety_factor` for thin-gap cases, as in the hardening thin-gap run;
- keep normal audit and coverage audit available in benchmark/reporting paths.

## Current Limitations

- Contact-band width and marker safety factor are sensitive parameters.
- Thin gaps should keep conservative marker safety settings.
- Far-field phi is intended for collision-oriented acceleration and is not a global high-accuracy SDF reconstruction guarantee.
- The current target is contact-oriented SDF sampling, not full-domain reconstruction.
- Marker timing still needs a future explicit split between accumulated block work and wall-clock critical-path time before excluding-marker speedup should be used as a headline metric.

## Next Steps

Recommended follow-up work:

1. Normalize marker timing so per-block accumulated time and wall-clock total are reported separately.
2. Add a default benchmark preset for thin-gap conservative settings.
3. Add a small gear-like benchmark case to CI or nightly validation if runtime remains modest.
4. Keep release promotion local until full CTest, Python unittest, install validation, alpha validation, clean check, and diff whitespace checks are green.

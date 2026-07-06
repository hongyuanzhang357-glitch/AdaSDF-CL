# AdaSDF-CL v1.16.0-alpha.3

## Summary

AdaSDF-CL v1.16.0-alpha.3 adds contact-focused narrow-band SDF construction
for collision-oriented use cases. It exact-samples the contact band,
interpolates far-field values, audits contact-band phi/sign/normal quality,
and reports end-to-end timing semantics.

The default builder path remains exact unless contact-band sampling is
explicitly requested with `--sampling contact-band`.

## Added

- Contact-band SDF sampling mode.
- Distance-aware and hybrid contact-band marker.
- Contact-band exact sampling with far-field interpolation.
- Contact-band quality audit for phi, sign and normal.
- Coverage audit for missed contact-band points.
- Marker cost audit and timing semantics.
- End-to-end timing fields and `performance_claim_allowed`.
- Synthetic fixtures for thin gaps, robot-like links, dense curved meshes and
  gear-like teeth.
- `adasdf_benchmark_contact_band_sampling`.
- `adasdf_sweep_contact_band_sampling`.
- Python wrapper support for `benchmark_contact_band_sampling`.

## Conservative Performance Wording

Allowed:

- Demonstrates end-to-end speedups on project-generated synthetic
  collision-oriented fixtures when contact-band quality and coverage gates
  pass.
- Contact-band quality is audited using phi, sign and normal metrics.
- End-to-end speedup includes marker cost.

Not allowed:

- Do not claim universal speedup for all STL models.
- Do not claim globally high-accuracy full-field SDF reconstruction.
- Do not present excluding-marker speedup as end-to-end performance.
- Do not claim industrial certification.

## Benchmark Summary

All benchmark rows use project-generated synthetic fixtures.

| case | speedup_end_to_end | quality | coverage | exact_node_ratio | notes |
| --- | ---: | --- | --- | ---: | --- |
| cube L5 | 1.41750 | PASS | PASS | 0.0866263 | project-generated closed cube release smoke |
| wavy L4 | 1.34531 | PASS | PASS | 0.123155 | project-generated wavy sphere |
| thin gap | 2.37600 | PASS | PASS | 0.0904199 | project-generated thin-gap box |
| robot link | 2.36958 | PASS | PASS | 0.0942893 | project-generated robot-like link |
| dense wavy | 2.83501 | PASS | PASS | 0.180487 | project-generated dense wavy sphere |

## Notes

- `speedup_end_to_end` is the release-facing performance metric.
- `speedup_excluding_marker`, `speedup_excluding_audit`, and
  `speedup_excluding_diagnostics` are diagnostic-only.
- Far-field phi is relaxed and should not be interpreted as globally
  high-accuracy SDF reconstruction.
- `.sdfbin` formats are unchanged.
- Older tags are preserved and must not be moved.

## Validation

Create this pre-release only after GitHub Actions are green for both `main` and
the `v1.16.0-alpha.3` tag.

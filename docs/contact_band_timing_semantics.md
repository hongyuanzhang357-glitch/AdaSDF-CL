# Contact-Band Timing Semantics

This document defines the timing vocabulary for the `v1.16.0-alpha.3`
contact-focused narrow-band sampling release.

## Primary Rule

`speedup_end_to_end` is the only timing field that may be used as an external
performance claim.

It is computed as:

```text
speedup_end_to_end =
    exact_reference_wall_time_ms / contact_band_wall_time_ms
```

By default, `contact_band_wall_time_ms` includes marker work, contact-band
sampling, interpolation, and required audit work. Report generation and
diagnostic bookkeeping are reported separately and are not part of the default
speedup.

## Timing Fields

| Field | Meaning | Claim use |
| --- | --- | --- |
| `exact_reference_wall_time_ms` | Wall time for the exact reference construction scope. | Denominator source for external comparison. |
| `contact_band_wall_time_ms` | Wall time for the contact-band construction scope, including marker and required audit by default. | Numerator target for external comparison. |
| `speedup_end_to_end` | Exact wall time divided by contact-band wall time. | Primary external metric. |
| `exact_reference_core_build_time_ms` | Exact reference core construction time. | Internal comparison. |
| `contact_band_core_build_time_ms` | Contact-band core construction time before audit/report diagnostics. | Internal comparison. |
| `speedup_core_build` | Core exact time divided by core contact-band time. | Diagnostic only. |
| `contact_band_marker_time_ms` | Wall-normalized marker contribution estimate. | Diagnostic only. |
| `contact_band_sampling_time_ms` | Wall-normalized exact/coarse sampling contribution estimate. | Diagnostic only. |
| `contact_band_interpolation_time_ms` | Wall-normalized interpolation contribution estimate. | Diagnostic only. |
| `contact_band_audit_time_ms` | Wall time spent in quality/coverage audit. | Diagnostic only. |
| `contact_band_report_time_ms` | Report output generation time when measured by caller. | Diagnostic only. |
| `contact_band_diagnostics_time_ms` | Wall time spent aggregating diagnostics. | Diagnostic only. |
| `speedup_excluding_audit` | Exact wall time divided by contact-band time with audit removed. | Diagnostic only. |
| `speedup_excluding_diagnostics` | Diagnostic-only speedup view with diagnostics excluded. | Diagnostic only. |
| `speedup_excluding_marker` | Exact wall time divided by contact-band time with marker contribution removed. | Diagnostic only. |

`marker_time_ms` remains a lower-level accumulated block-work diagnostic. It is
not the same as wall-clock marker time under parallel execution.

## Claim Gate

`performance_claim_allowed` is true only when:

```text
speedup_end_to_end > 1
&& contact_band_quality_passed
&& coverage_passed
&& contact_band_sign_mismatch_count == 0
&& near_surface_sign_mismatch_count == 0
&& normal_flip_count == 0
&& near_surface_normal_flip_count == 0
&& timing_mode == "end-to-end"
&& marker is included in speedup
&& audit is included in wall time
```

If marker or audit is explicitly excluded, the benchmark may still report a
diagnostic speedup, but `performance_claim_allowed` must remain false.

## External Wording

Allowed:

- "collision-oriented contact-band construction acceleration";
- "end-to-end contact-band benchmark speedup";
- "quality/coverage-gated contact-band speedup."

Not allowed:

- "global SDF reconstruction acceleration";
- "marker-excluded speedup" as a headline performance claim;
- "far-field phi has full global reconstruction accuracy."

The contact-band path is designed for collision-oriented SDF construction. Its
quality gate protects the zero-surface/contact-band region; far-field phi is a
relaxed rejection signal.

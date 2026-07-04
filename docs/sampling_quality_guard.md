# Sampling Quality Guard

The v1.16.0-alpha sampling quality guard decides whether a predicted block can
replace exact BVH sampling.

## Checks

For deterministic check points inside a block, the guard compares predicted phi
against exact TriangleBVH sampling and records:

- max absolute error;
- mean absolute error;
- RMS error;
- p95 absolute error;
- sign mismatch count;
- near-surface sign mismatch count.

## Acceptance

A prediction is rejected when error limits are exceeded or when sign consistency
fails. Rejected predictions fall back to exact sampling when fallback is
enabled, which is the default path used by the builder integration.

## Near-Surface Policy

Near-surface blocks remain exact by default. This protects sign consistency and
contact-candidate quality, where small phi errors matter most.

## Reporting

Hierarchical sampling stats are included in adaptive build reports and in the
hierarchical sampling benchmark outputs. The benchmark also supports
`--strict-json` for automation.

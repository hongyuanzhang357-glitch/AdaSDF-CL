# AdaSDF-CL v1.1.0-alpha

AdaSDF-CL v1.1.0-alpha adds ExpandedSDF accuracy auditing, sign mismatch
metrics, near-surface risk reporting, and a sampled existing-core expansion
bridge.

## Highlights

- New `ExpansionQuality` API for deterministic direct-vs-expanded audits.
- New `adasdf_expansion_quality` CLI with global/block expansion modes and CSV
  output.
- New sign mismatch, ambiguous sign, near-surface sign mismatch, fallback count,
  and fallback-rate metrics.
- Expanded `ExpansionOptions` for resolution, padding, near-surface band,
  sign epsilon, clamp, and fallback behavior.
- Benchmark CSV now includes quality fields in addition to timing fields.
- Query-mode demo can run `--quality-audit`.
- Existing-core `.sdfbin` models can be expanded through sampled `ExpandedSDF`
  layouts when direct query support is available.

## Validation Snapshot

- CPU-only build: PASS.
- CPU-only CTest: 51/51 PASS.
- Quality CLI smoke: PASS for demo adaptive global and block expansion.
- CUDA remains optional and is not required by the quality audit.

## Boundary

v1.1 expands models by sampling into global or block dense layouts. It does not
implement full low-rank compressed SDF GPU-native query. Existing-core tests
gracefully skip when the existing research core or fixture is unavailable.

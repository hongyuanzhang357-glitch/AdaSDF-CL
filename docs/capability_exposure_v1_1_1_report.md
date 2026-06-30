# AdaSDF-CL v1.1.1-alpha Capability Exposure Report

## Goal

Expose current AdaSDF-CL capabilities clearly without adding large algorithms
or overstating maturity.

## Added Files

- `tools/adasdf_capabilities.cpp`
- `examples/11_capability_walkthrough.cpp`
- `tests/test_capabilities_cli.cpp`
- `tests/test_capability_walkthrough_example.cpp`
- `tests/test_docs_capability_links.cpp`
- `docs/capability_audit_v1_1_1.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/fcl_complement_strategy.md`
- `docs/query_backend_matrix.md`
- `docs/contact_output_matrix.md`
- `docs/public_positioning.md`
- `docs/github_release_draft_v1_1_1_alpha.md`
- `docs/capability_exposure_v1_1_1_report.md`

## Modified Files

Version, README, CHANGELOG, alpha status, roadmap, tools/examples README, CMake
tool/example/test registration, validation scripts, and publication helper docs.

## Capability Matrix Summary

Implemented: core-free demo adaptive SDF, FCL-style collision API, contact
point/normal/depth, CPU query paths, expanded query paths, quality audit, sign
metrics, CMake package integration, and benchmark timing semantics.

Partial/experimental: demo surrogate, existing-core bridge, CUDA expanded query,
block-expanded local query, contact reduction/manifold behavior, and adaptive
builder bridge.

Planned: standalone arbitrary STL builder, mesh diagnostics/repair, FCL fallback
backend, CollisionWorld broadphase, CCD, Python, ROS/MoveIt, robot benchmarks,
and full low-rank GPU-native SDF query.

## FCL Complement Strategy

AdaSDF-CL is positioned as an FCL-style SDF backend under development. It should
complement FCL by adding signed distance, SDF normals, penetration depth,
batch query, CUDA expanded query, and quality audit. It is not a drop-in FCL
replacement.

## Query Backend Matrix

CPU none/global/block are supported. CUDA global/block are supported when CUDA
is enabled. CUDA none is not supported because CUDA requires pre-expanded data.

## Contact Output Matrix

Contact point, normal, penetration depth, signed distance, raw/reduced counts,
and max-contact cap are exposed. Stable robot-grade manifold clustering, contact
confidence, per-contact error bounds, and CCD remain planned.

## README Update Summary

The README now includes a short status table, links to the capability docs,
query-mode and contact-output summaries, accuracy-audit commands, CUDA timing
semantics, FCL complement positioning, build/install instructions, limitations,
roadmap, citation, and license.

## `adasdf_capabilities` Summary

The CLI prints version, position, implemented features, experimental/partial
features, planned features, and verbose pointers to capability docs. It has no
CUDA or existing-core dependency.

## Walkthrough Result

The walkthrough builds a demo adaptive model, runs a point query, runs a
collision query, prints contact fields, runs an expansion quality audit, and
prints p95/sign-mismatch metrics.

## Tests And Clean Check

Tests after this release: 54. Local validation results:

- CPU-only Release CTest: 54/54 PASS.
- CUDA Release CTest through an ASCII-only source junction: 54/54 PASS.
- Install validation: PASS.
- Alpha validation with CUDA enabled and install validation included: PASS.
- `adasdf_capabilities` smoke test: PASS.
- `check_repo_clean.py`: PASS.

`check_repo_clean.py` remains the source of truth for generated artifact
hygiene.

## Current Limits

No FCL backend, ROS/MoveIt, CCD, mesh repair, standalone arbitrary STL builder,
or full low-rank GPU-native query is implemented in this release.

## Next Steps

Add standalone STL diagnostics, start FCL fallback design, define CollisionWorld
broadphase scope, and add contact confidence/error estimates.

# AdaSDF-CL v1.1.1 Capability Audit

## Scope

This audit summarizes the public repository as of v1.1.1-alpha. It separates
implemented, partial, experimental, demo-only, existing-core-only, and planned
capabilities. It is a positioning audit, not a new algorithm milestone.

## Public API Capabilities

- `SDFModel`, analytic/demo adaptive SDF models, and `.sdfbin` readers/writers.
- FCL-style `CollisionObject`, `CollisionRequest`, `CollisionResult`,
  `DistanceRequest`, `DistanceResult`, `collide()`, and `distance()`.
- Contact output with point, normal, penetration depth, signed distance,
  object ids, feature ids, gradient, raw count, reduced count, and max-contact
  cap.
- Batch query API for CPU signed distance, gradients, and normals.
- Query backend and expansion APIs: CPU direct, CPU global expanded, CPU block
  expanded, CUDA global expanded, CUDA block expanded.
- `ExpandedSDF`, `SDFExpander`, `ExpansionQuality`, and sign mismatch helpers.
- Optional CUDA resident expanded-SDF query backend.

## CLI Tools

- `adasdf_capabilities`
- `adasdf_recommend_demo`
- `adasdf_build_demo_adaptive`
- `adasdf_make_demo_box`
- `adasdf_info`
- `adasdf_query`
- `adasdf_collide`
- `adasdf_collide_boxes_demo`
- `adasdf_query_mode_demo`
- `adasdf_expansion_quality`
- `adasdf_benchmark_batch_query`
- Existing-core-dependent `adasdf_build`

## Examples

Core-free working examples include demo adaptive collision, surrogate adaptive
workflow, SVG collision view, downstream CMake integration, and the v1.1.1
capability walkthrough. Existing-core examples demonstrate load/query,
two-object collision, FCL-style API shape, build-then-query, and contact
reduction when the existing core and fixtures are available.

## Query Modes

CPU direct is the default stable route. CPU expanded modes validate and emulate
expanded layouts. CUDA is optional and only supports pre-expanded global/block
dense layouts. CUDA compressed-direct query remains planned.

## Collision And Contact Output

Implemented contact output includes contact point, normal, penetration depth,
signed distance, raw/reduced counts, object/feature ids, and deterministic
contact reduction. Stable robot-grade manifold clustering, confidence, per
contact error estimates, and CCD remain planned.

## GPU / CUDA

Implemented when CUDA is enabled: expanded global/block resident upload,
phi-only and phi+normal queries, warmup/repeat benchmark semantics,
kernel-only timing, device-only benchmark timing, workspace reuse, and CPU/GPU
alignment tests. CUDA does not yet perform compressed-direct low-rank SDF query
or GPU pair-collision solving.

## ExpandedSDF Quality Audit

Implemented: max/mean/RMS/p95 error, strict sign mismatch, ambiguous sign,
near-surface sign mismatch, fallback count, fallback rate, deterministic sample
generation, and `adasdf_expansion_quality` CSV output.

## Existing-Core Bridge

Existing-core `.sdfbin` read/query and STL-to-sdfbin build paths are available
only when the existing research core and dependencies are present. The v1.1
sampled expansion bridge can turn any direct-query `SDFModel` into an
`ExpandedSDF`; CI skips existing-core-only tests when unavailable.

## External CMake Integration

Implemented: install/export targets, `find_package(AdaSDFCL CONFIG REQUIRED)`,
package smoke test, and downstream CMake example.

## Current Limitations

- Not a drop-in ABI-compatible replacement for FCL.
- No public standalone arbitrary STL adaptive builder yet.
- No mesh diagnostics or mesh repair.
- No CollisionWorld broadphase.
- No CCD.
- No Python, ROS, or MoveIt integration.
- No full low-rank GPU-native SDF query.

## External Feedback Mapping

Already implemented: FCL-style pair collision API, contact point/normal/depth,
max contacts, contact reduction, signed distance, CPU/CUDA batch query,
expanded quality audit, SVG contact visualization, and external CMake
integration.

Partially implemented: existing-core real asset bridge, adaptive builder,
surrogate recommender, block-expanded query, contact manifold behavior, and
FCL-style adapter surface.

Not implemented: FCL fallback backend, broadphase CollisionWorld, CCD, mesh
repair, standalone arbitrary STL builder, ROS/MoveIt, Python package, and robot
benchmark suite.

## Recommended Priority

1. Public standalone STL diagnostics and builder entry points.
2. FCL fallback / hybrid backend design.
3. CollisionWorld broadphase.
4. Contact manifold clustering and confidence/error estimates.
5. Python and robot benchmark integration.

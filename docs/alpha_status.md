# Alpha Status

AdaSDF-CL 1.0.0-alpha is a research-preview release candidate.

## What Works

- Core-free analytic box SDF backend.
- Demo surrogate recommender.
- Demo adaptive builder with octree, block, and rank metadata.
- Demo adaptive `.sdfbin` read/write with `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- Point query and collision query over demo adaptive models.
- `CollisionRequest::max_contacts` and CLI `--max-contacts` enforcement.
- SVG collision visualization.
- Installable CMake package with downstream no-input demo adaptive path.
- CPU batch query for signed distance, gradient, and normal data.
- Optional CUDA batch query backend for core-free analytic/demo adaptive boxes.
- Deterministic 10k/100k/1M benchmark CLI with CSV output.
- Existing-core bridge remains available when configured separately.

## Current Boundary

The v0.9 demo surrogate is not universal, not fully trained, and not an optimality guarantee. It is a deterministic public workflow exerciser.

The demo adaptive builder uses analytic box SDF queries and demo metadata. It is not the full adaptive compressed STL-to-SDF builder.

The v1.0 CUDA path is optional and intentionally narrow. It supports batch query kernels for core-free analytic/demo adaptive boxes first. It is not a full low-rank compressed SDF GPU expansion and is not an industrial GPU collision pipeline.

## Validation Snapshot

- Expected tests: 31.
- Expected install validation: PASS with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected alpha validation: PASS.
- Expected clean check: PASS.
- Target external collision test verdict for v0.9.0-alpha: PASS for the demo adaptive workflow.
- Expected CUDA-unavailable behavior: GPU benchmark/tests SKIPPED, not FAILED.

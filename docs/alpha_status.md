# Alpha Status

AdaSDF-CL 1.0.2-alpha is a research-preview release candidate.

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
- Query-mode configuration for backend, expansion mode, block selection,
  resident expanded data, and CPU fallback policy.
- CPU direct, CPU global-expanded, CPU block-expanded, CUDA global-expanded,
  and CUDA block-expanded `QueryEngine` paths.
- Optional CUDA resident expanded SDF backend for core-free analytic/demo
  adaptive boxes.
- CUDA query workspace reuse for benchmark repeats over resident expanded SDF data.
- CUDA phi-only expanded SDF kernel for signed-distance-only kernel timing.
- Deterministic 10k/100k/1M benchmark CLI with CSV output, memory columns,
  setup/kernel/total timings, detailed timing breakdowns, warmup/repeat
  statistics, fallback counts, and error columns.
- Existing-core bridge remains available when configured separately.

## Current Boundary

The v0.9 demo surrogate is not universal, not fully trained, and not an optimality guarantee. It is a deterministic public workflow exerciser.

The demo adaptive builder uses analytic box SDF queries and demo metadata. It is not the full adaptive compressed STL-to-SDF builder.

The v1.0.2 CUDA path is optional and intentionally narrow. It supports batch queries only after SDF data has been pre-expanded into global or block dense data. `CUDA + None` is rejected because compressed-direct GPU query is not implemented. This is not a full low-rank compressed SDF GPU expansion and is not an industrial GPU collision pipeline.

Benchmark `total_ms` is a full query timing. Benchmark `kernel_ms` is CUDA kernel event timing. Original UI warmed kernel-average numbers should be compared to `--kernel-only --phi-only --reuse-resident` rows.

## Validation Snapshot

- Expected tests: 41.
- Expected install validation: PASS with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected alpha validation: PASS.
- Expected clean check: PASS.
- Target external collision test verdict for v0.9.0-alpha: PASS for the demo adaptive workflow.
- Expected CUDA-unavailable behavior: GPU benchmark/tests SKIPPED, not FAILED.
- Current v1.0.2 local CPU CTest result: 41/41 PASS.
- Current v1.0.2 local CUDA CTest result: 41/41 PASS when CUDA is enabled through an ASCII-only source/build path.

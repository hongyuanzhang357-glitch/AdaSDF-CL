# AdaSDF-CL v1.0.3-alpha Performance Optimization Plan

## Goal

Reduce the gap between:

- phi-only kernel time;
- full query total time;
- block-expanded kernel time.

v1.0.3-alpha should build on v1.0.2-alpha benchmark semantics without mixing
kernel-only UI comparisons with full query total-time comparisons.

## Focus Areas

### 1. Block Lookup Acceleration

- Replace linear block scan with a deterministic block index map.
- Reuse center block id for finite-difference neighbor samples.
- Avoid scanning blocks seven times per point.
- Preserve deterministic fallback behavior for points outside selected blocks.

### 2. Phi / Normal Split

- Expose phi-only, normal-only, and phi+normal modes.
- Benchmark each mode separately.
- Keep phi-only as the primary comparison point for original UI kernel-average
  timings.
- Keep phi+normal as the current full query correctness path.

### 3. Device-Only Output Mode

- Allow query results to remain on GPU.
- Avoid D2H copies when a downstream pipeline can consume GPU buffers.
- Report device-only timing separately from host-output timing.
- Keep host-output mode as the default public API behavior.

### 4. Host/Device Buffer Reuse

- Reuse CPU output buffers.
- Reuse pinned host memory.
- Reuse device query buffers.
- Keep one-shot query behavior available for simple callers.

### 5. Memory Layout

- Evaluate AoS vs SoA for points and normals.
- Improve coalesced reads and writes.
- Measure packing/postprocess costs separately from kernel costs.
- Keep double precision as the default correctness mode unless an explicit
  float-mode experiment is added.

### 6. Benchmark Reporting

- Keep kernel-only and total-time both visible.
- Never mix kernel-only UI comparisons with total query comparisons.
- Report warmup/repeat settings with every benchmark row.
- Keep `max_abs_phi_error` and `max_normal_error` semantics explicit for
  phi-only and phi+normal rows.

## Expected Deliverables

- Faster CUDA block mode.
- Reduced full query total time.
- More stable benchmark repeat statistics.
- Clear benchmark table for:
  - CPU none;
  - CPU global;
  - CUDA global phi-only;
  - CUDA global phi+normal;
  - CUDA block phi-only;
  - CUDA block phi+normal.

## Non-Goals

- Do not move v1.0.2-alpha or older tags.
- Do not implement full low-rank compressed SDF GPU-native query in this plan.
- Do not claim full contact-pipeline parity from phi-only kernel timings.
- Do not hide total query time when reporting kernel-only improvements.

## First Candidate Task

Fix benchmark CLI test portability on Linux by invoking the benchmark executable
through a resolved path or an explicit `./` prefix when CTest sets the working
directory to the benchmark target directory. This is a release-blocking CI
portability fix, separate from the v1.0.3 performance work.

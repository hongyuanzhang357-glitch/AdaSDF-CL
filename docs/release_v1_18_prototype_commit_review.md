# v1.18 Prototype Commit Review

Base: `bf2bfc21626138066e7d5768fe13d9d83ffde802` (`origin/main`).

Policy: migrate generic implementation, public headers, formal tools, schemas,
and tests. Do not migrate `docs/local_*`, local scripts, generated outputs, or
advisor/optimizer experiments. The entries below record the reviewed file
boundary; the release was assembled as a curated patch, not by merging the
prototype branch.

## f206dfc

- Purpose: mixed-level adaptive leaves, surface-aware refinement, adaptive
  tree statistics, CLI reporting, and regression tests.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`,
  `include/adasdf/generation/AdaptiveBlockSDFBuilder.h`,
  `include/adasdf/generation/AdaptiveOctreeBuilder.h`,
  `include/adasdf/generation/AdaptiveTreeStats.h`,
  `include/adasdf/generation/AdaptiveTreeStatsWriter.h`,
  `include/adasdf/profile/BuildProfileCounters.h`,
  `schemas/adasdf.build_profile.v1.schema.json`,
  `src/generation/AdaptiveBlockSDFBuildReportWriter.cpp`,
  `src/generation/AdaptiveBlockSDFBuilder.cpp`,
  `src/generation/AdaptiveOctreeBuilder.cpp`,
  `src/generation/AdaptiveTreeStats.cpp`,
  `src/generation/AdaptiveTreeStatsWriter.cpp`,
  `src/profile/BuildProfileJsonWriter.cpp`,
  `tests/test_adaptive_tree_stats.cpp`,
  `tests/test_adaptive_tree_stats_writer.cpp`,
  `tests/test_build_profile_json_cli.cpp`,
  `tests/test_diagnose_adaptive_tree_cli.cpp`,
  `tests/test_mixed_level_block_lookup.cpp`,
  `tests/test_mixed_level_leaf_builder.cpp`, `tools/BuildCliProfileHelpers.h`,
  `tools/adasdf_build_adaptive_sdf.cpp`,
  `tools/adasdf_build_compressed_sdf.cpp`, and
  `tools/adasdf_diagnose_adaptive_tree.cpp`.
- Excluded: both `docs/local_mixed_level_*` reports.

## 499aabb

- Purpose: reusable near-surface sampling and SDF accuracy audit.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`,
  `include/adasdf/audit/NearSurfaceSampleGenerator.h`,
  `include/adasdf/audit/SDFAccuracyAudit.h`,
  `include/adasdf/audit/SDFAccuracyReportWriter.h`, matching three `src/audit`
  implementations, `tests/test_audit_sdf_accuracy_cli.cpp`,
  `tests/test_near_surface_sample_generator.cpp`,
  `tests/test_sdf_accuracy_audit.cpp`, and
  `tools/adasdf_audit_sdf_accuracy.cpp`.
- Excluded: `docs/local_horse_L3_near_surface_accuracy_report.md`.

## fff942a

- Purpose: local Horse quality sweep orchestration and report.
- Public-stable: no.
- Prototype only: `scripts/local_horse_quality_sweep.ps1` and
  `docs/local_horse_quality_sweep_report.md` are geometry-specific evidence.

## c1bbd3d

- Purpose: generic sign-cluster audit, exact-mode comparison, and contact-band
  diagnostics derived from the Horse investigation.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`, audit/sampling headers and sources,
  `tests/test_contact_band_mask_sensitivity.cpp`,
  `tests/test_mesh_sign_diagnostic.cpp`,
  `tests/test_near_surface_exact_modes.cpp`,
  `tests/test_sign_cluster_audit.cpp`,
  `tools/adasdf_audit_sdf_accuracy.cpp`,
  `tools/adasdf_build_compressed_sdf.cpp`,
  `tools/adasdf_diagnose_contact_band_mask.cpp`, and
  `tools/adasdf_diagnose_mesh_sign.cpp`.
- Excluded: `docs/local_horse_sign_mismatch_diagnosis_report.md`.

## 95cda0a

- Purpose: contact-band coverage audit, provenance, and coverage-driven
  refinement.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`, all four
  `include/adasdf/coverage/*` headers, `include/adasdf/provenance/BlockProvenance.h`,
  matching `src/coverage/*` and `src/provenance/BlockProvenance.cpp`,
  `tests/test_audit_query_source_explain.cpp`,
  `tests/test_block_provenance.cpp`,
  `tests/test_contact_band_coverage_audit.cpp`,
  `tests/test_coverage_driven_refinement.cpp`, and the affected build/audit/info
  tools.
- Excluded: `docs/local_horse_coverage_refinement_report.md`.

## 53ba181

- Purpose: near-zero compression guard, dense-contact fallback, counters, and
  audit tooling.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`,
  `include/adasdf/compression/BlockLowRankCompressor.h`,
  `include/adasdf/profile/BuildProfileCounters.h`, corresponding compression
  and profile sources, three compression regression tests,
  `tools/BuildCliProfileHelpers.h`,
  `tools/adasdf_audit_compression_near_surface.cpp`, and affected adaptive and
  compressed build tools.
- Excluded: `docs/local_horse_dense_vs_compressed_sign_report.md`.

## b63a850

- Purpose: reusable mismatch-cell forensics and local exact subcell probes.
- Public-stable: yes.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`, four
  `include/adasdf/audit/*` diagnostic headers, their four source files,
  `tests/test_cell_zero_crossing_diagnostic.cpp`,
  `tests/test_local_exact_subcell_probe.cpp`,
  `tests/test_mismatch_cell_forensics.cpp`, and
  `tools/adasdf_diagnose_mismatch_cells.cpp`.
- Excluded: `docs/local_horse_mismatch_cell_forensics_report.md`.

## da0cb35

- Purpose: initial decoupled narrow-band sampling and compression-brick
  compiler path.
- Public-stable: yes, as an explicitly bounded alpha preview.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`, the twelve
  initial `include/adasdf/narrowband/*` headers, matching narrow-band sources,
  five formal narrow-band tests, and
  `tools/adasdf_build_narrowband_brick_sdf.cpp`.
- Excluded: `docs/local_narrowband_brick_sdf_generator_report.md`.

## e89d7c9

- Purpose: narrow-band block index/query, contact-exact enrichment, sign guard,
  and sparse-query integration.
- Public-stable: yes, as part of the same alpha preview.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`, affected
  narrow-band headers/sources, `NarrowBandBrickIndex` and
  `NarrowBandBrickQuery`, five formal tests,
  `tools/adasdf_benchmark_sparse_query.cpp`,
  `tools/adasdf_build_narrowband_brick_sdf.cpp`, and
  `tools/adasdf_generate_aabb_samples.cpp`.
- Excluded: `docs/local_narrowband_brick_queryfix_report.md`.

## c5cb31f

- Purpose: decouple sampling-octree depth from compression-brick tensor
  dimensions and formalize mapping policies.
- Public-stable: yes, as part of the same alpha preview.
- Cherry-pick files: `CMakeLists.txt`, `include/adasdf/adasdf.h`,
  `BrickTensorDimensionPolicy` header/source, affected narrow-band
  headers/sources, `tests/test_brick_tensor_dimension_policy.cpp`,
  `tests/test_sampling_to_brick_mapper.cpp`, affected existing narrow-band
  tests, and `tools/adasdf_build_narrowband_brick_sdf.cpp`.
- Excluded: `docs/local_decoupled_sampling_brick_generator_report.md`.

## d73b332

- Purpose: record a local Horse revalidation run.
- Public-stable: no.
- Prototype only: it changes only
  `docs/local_decoupled_sampling_brick_generator_report.md`.

## Uncommitted compiler curation

The release additionally includes the stable constrained compiler contract,
exact oracle, adaptive tensor codecs, constrained format/model/builder,
`adasdf_create_sdf`, one formal creation benchmark, and thirteen formal tests.
The Advisor implementation was deliberately reduced to the deterministic
Heuristic path. Learned V1/V2 code, model loading, preview candidate tooling,
three experiment benchmarks, V1-V9 scripts/reports, optimizer directories,
datasets, artifacts, weights, caches, and generated figures were excluded.

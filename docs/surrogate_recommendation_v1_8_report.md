# AdaSDF-CL v1.8.0-alpha Surrogate-Guided Recommendation Report

## Goal

v1.8.0-alpha adds the first public, core-free, deterministic build
recommender for choosing between DenseSDF, AdaptiveBlockSDF, and
CompressedAdaptiveBlockSDF build paths.

The recommender accepts a mesh, target near-surface error, memory budget,
intended use case, signed/unsigned policy, dense fallback policy, compression
preference, and an optional lightweight profile. It emits a recommended recipe,
estimated memory/error/cost, confidence, rationale, warnings, Markdown and
JSON-like reports, and a copyable build CLI command. It does not execute the
build by default.

## Added Files

- `include/adasdf/recommendation/BuildRecommendationTypes.h`
- `include/adasdf/recommendation/MeshFeatureExtractor.h`
- `include/adasdf/recommendation/BuildSurrogateEstimator.h`
- `include/adasdf/recommendation/BuildRecommender.h`
- `include/adasdf/recommendation/BuildRecommendationWriter.h`
- `include/adasdf/recommendation/SurrogateProfile.h`
- `src/recommendation/MeshFeatureExtractor.cpp`
- `src/recommendation/BuildSurrogateEstimator.cpp`
- `src/recommendation/BuildRecommender.cpp`
- `src/recommendation/BuildRecommendationWriter.cpp`
- `src/recommendation/SurrogateProfile.cpp`
- `tools/adasdf_recommend_build.cpp`
- `examples/20_build_recommendation_demo.cpp`
- `tests/test_mesh_feature_extractor.cpp`
- `tests/test_build_surrogate_estimator.cpp`
- `tests/test_surrogate_profile.cpp`
- `tests/test_build_recommender.cpp`
- `tests/test_build_recommendation_writer.cpp`
- `tests/test_recommend_build_cli.cpp`
- `tests/test_recommendation_workflow.cpp`
- `tests/test_adaptive_builder_preview_v1_8.cpp`
- `models/surrogate/v1_8/README.md`
- `models/surrogate/v1_8/default_profile.txt`
- `docs/surrogate_guided_recommendation.md`
- `docs/build_recommendation_schema.md`
- `docs/recommendation_profiles.md`
- `docs/recommended_build_workflow.md`
- `docs/github_release_draft_v1_8_0_alpha.md`

## Modified Files

- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `CMakeLists.txt`
- `cmake/adasdf_cl_version.cmake`
- `include/adasdf/adasdf.h`
- `include/adasdf/version.h`
- `include/adasdf/generation/AdaptiveSDFBuilderPreview.h`
- `src/generation/AdaptiveSDFBuilderPreview.cpp`
- `src/generation/AdaptiveBlockSDFBuildReportWriter.cpp`
- `src/compression/CompressionReportWriter.cpp`
- `tools/adasdf_build_dense_sdf.cpp`
- `tools/adasdf_build_adaptive_sdf.cpp`
- `tools/adasdf_build_compressed_sdf.cpp`
- `tools/adasdf_build_adaptive_sdf_preview.cpp`
- `tools/adasdf_capabilities.cpp`
- `tools/adasdf_info.cpp`
- `scripts/run_install_validation.py`
- `scripts/run_alpha_validation.py`
- `examples/README.md`
- `docs/alpha_status.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/roadmap.md`
- `docs/public_positioning.md`
- `docs/adaptive_builder_preview.md`
- `docs/low_rank_block_compression.md`
- `docs/stl_to_compressed_sdf_workflow.md`
- `docs/compression_quality_metrics.md`
- `docs/dense_sdf_builder.md`
- `docs/stl_to_sdf_public_workflow.md`
- `docs/stl_to_adaptive_sdf_workflow.md`
- `docs/surrogate_recommendation.md`

## BuildTarget Design

`BuildTarget` records the user-facing recommendation intent:

- `target_near_surface_error`
- `memory_budget_mb`
- `use_case`: `contact`, `balanced`, `quality`, or `memory-saving`
- signed/unsigned behavior
- open-mesh unsigned fallback permission
- dense fallback permission
- compression preference
- fast-build preference
- optional build-time hint

The type is plain C++ and has no Python, JSON, ML runtime, existing-core, FCL,
or CUDA dependency.

## MeshFeatureExtractor Design

`MeshFeatureExtractor` converts project STL/mesh diagnostics into a compact
`MeshFeatureSummary`. It reports vertex/triangle counts, AABB diagonal and
volume, watertight/open-mesh state, boundary and non-manifold counts,
degenerate/duplicate triangle counts, connected component count, readiness
score, and warnings.

Read failures return `valid=false` with an error warning rather than throwing.
Degenerate bounds are guarded so feature values stay finite.

## BuildSurrogateEstimator Design

`BuildSurrogateEstimator` is a deterministic heuristic-calibrated estimator. It
is deliberately not a universal trained model. It generates a bounded candidate
set for DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF, then
estimates memory, near-surface error, compression ratio, build cost, confidence,
and score.

## SurrogateProfile Design

`SurrogateProfile` loads lightweight key-value constants such as
`memory_safety_factor`, `error_safety_factor`, `max_max_level`,
`max_block_resolution`, and rank bounds. Unknown keys and invalid values become
warnings and fall back to defaults. The profile is not a trained model file and
does not require JSON, joblib, sklearn, PyTorch, TensorFlow, or Python.

## Candidate Generation

Candidate generation is deterministic and bounded. It considers:

- dense resolutions for direct DenseSDF builds;
- adaptive octree levels and block resolutions;
- compressed adaptive variants with rank bounds;
- dense fallback policy;
- compression preference;
- open mesh and signed/unsigned policy;
- use-case-specific preferences.

`memory-saving` strongly favors compressed adaptive candidates. `contact`
prefers near-surface accuracy. `quality` allows higher resolution candidates.

## Memory Estimate

Dense memory is estimated as:

```text
dense_resolution^3 * 8 bytes * memory_safety_factor
```

Adaptive memory uses a deterministic surface-block heuristic based on mesh size,
AABB scale, max level, block resolution, and safety factor.

Compressed adaptive memory estimates matrix-SVD block storage using:

```text
nx * rank + rank + rank * (ny * nz)
```

plus metadata and safety factors.

## Error Estimate

Spatial error is estimated from AABB diagonal, octree level, and block
resolution. Compressed candidates also consider target compression error. The
reported near-surface estimate is the larger of spatial and compression
components after safety factors.

## Scoring Logic

Scores reward meeting the memory budget and target error, then apply use-case
preferences:

- contact: lower near-surface error and safe compression settings;
- balanced: memory/error tradeoff;
- quality: higher level and resolution;
- memory-saving: compressed adaptive path;
- prefer-fast-build: lower build-cost candidates;
- prefer-compression: compressed adaptive path.

Infeasible candidates remain visible with warnings, but feasible candidates are
ranked first.

## Confidence Logic

Confidence is based on mesh validity, readiness, estimate feasibility, and how
close memory/error estimates are to the requested limits. Low-readiness meshes,
open meshes requested as signed, and extreme budgets reduce confidence and add
warnings.

## BuildRecommender Design

`BuildRecommender` ties together STL loading, diagnostics, readiness,
feature extraction, candidate generation, estimation, ranking, and command
emission. It supports both `recommendFromMesh` and `recommendFromSTL`.

Open meshes with unsigned fallback enabled can emit unsigned recommendations.
Low-readiness meshes add cleanup guidance. Recommendation reports always include
model-boundary limitations.

## adasdf_recommend_build CLI

The CLI supports:

- input STL
- `--target-error`
- `--memory-mb`
- `--use-case contact|balanced|quality|memory-saving`
- `--signed`
- `--unsigned`
- `--allow-open-unsigned`
- `--allow-dense-fallback`
- `--no-dense-fallback`
- `--prefer-compression`
- `--no-prefer-compression`
- `--prefer-fast-build`
- `--profile`
- `--out`
- `--json`
- `--emit-command`
- `--verbose`

It returns 0 for normal recommendation, 1 for invalid input mesh reads, and 2
when no feasible candidate can be recommended.

## Report Schema

Markdown reports include summary, target, mesh features, recommended path,
recommended parameters, memory/error/ratio/cost estimates, confidence,
rationale, warnings, candidate table, CLI command, limitations, and validation
commands.

The JSON-like report includes the same core fields without depending on an
external JSON library.

## Examples

`examples/20_build_recommendation_demo.cpp` generates a small temporary cube
STL, builds a `BuildTarget`, runs `BuildRecommender`, and prints the recommended
path, confidence, estimated memory/error, and CLI command. It does not generate
`.sdfbin` output.

## Tests

New tests cover:

- mesh feature extraction;
- surrogate estimator candidate generation and scoring;
- profile loading and fallback behavior;
- high-level recommender behavior;
- Markdown and JSON-like report writing;
- CLI behavior and generated reports;
- recommendation workflow smoke;
- v1.8 adaptive builder preview wording.

## Validation Results

- CPU-only configure/build: PASS.
- CPU-only CTest: PASS, 101/101.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean repo check: PASS.
- CUDA-enabled configure/build/CTest: PASS, 101/101, when run through an
  ASCII-only `subst` path. Direct CUDA configure under the original Chinese path
  fails in NVCC/MSBuild compiler identification because the generated object
  path is mis-decoded.
- `adasdf_recommend_build` CLI smoke: PASS, produced Markdown and JSON-like
  reports in the build directory and emitted a copyable build command.

## Current Limitations

- Experimental deterministic recommender.
- Not a universal trained model.
- Not fully trained.
- Not an optimality guarantee.
- No external training pipeline in v1.8.
- No Python/sklearn/joblib/PyTorch/TensorFlow runtime dependency.
- No large model artifact or external training dataset.
- Recommendation does not run the build by default.
- Generated SDFs should still be validated with `adasdf_info`,
  `adasdf_expansion_quality`, benchmark tools, and collision/query smoke tests.
- GPU-native compressed query remains planned work.
- FCL fallback and CollisionWorld broadphase remain planned work.

## Next Steps

- Add trained surrogate integration once real benchmark/quality datasets exist.
- Add online calibration from generated build and quality reports.
- Add a Python wrapper or binding without making the C++ recommender depend on
  Python.
- Add GPU-native adaptive/compressed query paths.
- Add FCL hybrid fallback and CollisionWorld broadphase integration.

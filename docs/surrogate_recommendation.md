# DASH-SDF Surrogate Recommendation

AdaSDF-CL v0.4+ includes the historical public surface for DASH-SDF
surrogate-guided build recommendation, but does not embed or execute a trained
external surrogate model.

AdaSDF-CL v1.8.0-alpha adds a separate public core-free deterministic build
recommender through `adasdf_recommend_build` and `adasdf::BuildRecommender`.
That v1.8 path is heuristic-calibrated, not a universal trained model, not
fully trained, and not an optimality guarantee. See
`docs/surrogate_guided_recommendation.md`.

## Public API

```cpp
adasdf::BuildOptions constraints;
constraints.tau_near_abs = 1e-4;
constraints.memory_limit_mb = 256.0;
constraints.block_memory_limit_mb = 128.0;
constraints.use_surrogate_recommendation = true;

auto recommendations =
    adasdf::SurrogateRecommender::recommend("part.stl", constraints, 5);
```

Each `SurrogateRecommendation` contains:

- `options`: the proposed `BuildOptions`.
- `predicted_p95_error`: reserved prediction field.
- `predicted_memory_mb`: reserved prediction field.
- `safety_score`: reserved safety field.
- `credible`: whether the recommendation came from an available validated backend.
- `note`: human-readable status.

## v0.4+ Behavior

`SurrogateRecommender::isAvailable()` returns `false`. `recommend()` returns one recommendation with the caller's constraints, `credible=false`, and this note:

```text
Surrogate recommendation backend is not available in this build.
```

This is intentional. AdaSDF-CL should not invent predicted p95 error, memory use, or safety scores when no compiled surrogate backend is present.

## Validation Summary Imported from External DASH-SDF Work

The external GeneralSTLSDFSurrogate validation card was reviewed and summarized in AdaSDF-CL docs, but no large artifacts were copied. The current AdaSDF-CL repository does not include:

- Python/joblib model files.
- raw STL datasets.
- generated experiment directories.
- intermediate NumPy arrays.
- large logs or local machine paths.

Summary metrics from the reviewed validation card:

- Dataset: 20 approved STL models.
- Training samples: 100 base samples plus 13 active-learning samples.
- Top-K real validations: 34 earlier validations plus 23 v1.3 validations.
- Backend failures: 0 reported in the v1.2 validation card.
- p95 prediction R2: 0.964526.
- memory prediction R2: 0.999489.
- sign-mismatch recall: 0.912281.
- false-safe rate: 0.000000.
- Top-K STL-level success rate: Top-1 0.8, Top-3 1.0, Top-5 1.0.
- candidate-level success rate: 0.826087.
- credibility conclusion: credible within the validation-card limits.

## Integration Rules

A future real integration should:

- Load an explicitly packaged surrogate artifact from a user-provided path or install-time resource.
- Validate feature schema and model metadata before returning `credible=true`.
- Keep safety-calibration thresholds visible in metadata.
- Refuse recommendations when the requested mesh class, units, constraints, or feature schema fall outside validation coverage.
- Keep heavyweight training data and generated experiment outputs outside the AdaSDF-CL source tree.

Until then, `--use-surrogate` in `adasdf_build` is a safe no-op with an explanatory fallback.

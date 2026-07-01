# Surrogate-Guided Build Recommendation

AdaSDF-CL v1.8.0-alpha adds a core-free deterministic build recommender through
`adasdf_recommend_build` and the `adasdf::BuildRecommender` C++ API.

The recommender reads an STL, runs mesh diagnostics/readiness checks, extracts a
compact mesh feature summary, scores DenseSDF, AdaptiveBlockSDF, and
CompressedAdaptiveBlockSDF candidates, and emits a copyable build command.

This feature is experimental. It is not a universal trained model, not fully
trained, and not an optimality guarantee. It does not run the build by default.
Always validate the final SDF with the generated build, compression, and quality
reports.

## CLI

```bash
adasdf_recommend_build model.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --json recommendation.json --emit-command
```

Supported use cases are:

- `contact`
- `balanced`
- `quality`
- `memory-saving`

The input controls include signed/unsigned output, open-mesh unsigned fallback,
dense fallback policy, compression preference, fast-build preference, and an
optional `--profile` file.

## Outputs

The report includes:

- recommended build path;
- dense/adaptive/compressed parameters;
- estimated memory;
- estimated near-surface error;
- estimated compression ratio;
- estimated build cost score;
- confidence;
- warnings;
- rationale;
- candidate table;
- copyable build command.

## Boundaries

v1.8.0-alpha uses deterministic heuristic-calibrated constants. It does not
load Python, sklearn, joblib, PyTorch, TensorFlow, external training data, or a
large model file.

Full low-rank compressed SDF GPU-native query, trained model integration,
online calibration, pybind11 bindings, FCL fallback, and CollisionWorld
broadphase remain planned work.
